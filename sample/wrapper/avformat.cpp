#include "avformat.h"

AVFormat::AVFormat(const std::string& url)
    : url_(url) {
    avformat_network_init();
    LoadStreams();
}

AVFormat::~AVFormat() {
    avformat_network_deinit();
}

bool AVFormat::LoadStreams() {
    AVFormatContext *context = nullptr;
    if (avformat_open_input(&context, url_.c_str(), NULL, NULL) < 0) {
        fprintf(stderr, "Could not open input file '%s'\n", url_);
        return false;
    }

    if (avformat_find_stream_info(context, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&context);
        return false;
    }

    context_ = std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>>(
        context, [](AVFormatContext* ctx) {
        avformat_close_input(&ctx);
    });
    return true;
}

void AVFormat::DumpStreams() const {
    if (!context_)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    av_dump_format(context_.get(), 0, url_.c_str(), 0);
}

int AVFormat::NumOfStreams() const {
    if (!context_)
        return 0;

    return context_->nb_streams;
}

std::shared_ptr<AVStream> AVFormat::GetStream(int index) const {
    if (!context_)
        return nullptr;

    if (index < 0 || index >= context_->nb_streams) {
        return nullptr;
    }

    std::shared_ptr<AVStream> stream(context_->streams[index], [](AVStream* p) {});
    return stream;
}

std::shared_ptr<AVPacket> AVFormat::ReadNextPacket() const {
    if (!context_)
        return nullptr;

    std::shared_ptr<AVPacket> pkt(new AVPacket, [&](AVPacket* p) {
        std::lock_guard<std::mutex> lock(mutex_);
        av_packet_unref(p);
        delete p;
    });

    std::lock_guard<std::mutex> lock(mutex_);
    if (av_read_frame(context_.get(), pkt.get()) < 0)
        return nullptr;

    return pkt;
}

bool AVFormat::SaveFormat(const std::string& url) {
    if (!context_)
        return false;

    AVFormatContext* output_ctx = nullptr;
    if (avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, url.c_str()) < 0)
        fprintf(stderr, "Failed to create output context\n");
        return false;

    for (int i = 0; i < context_->nb_streams; i++) {
        AVStream *stream = context_->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("Found video stream at index %d\n", i);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("Found audio stream at index %d\n", i);
        }
    }

    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, url.c_str(), AVIO_FLAG_WRITE) < 0)
            throw std::runtime_error("Failed to open output file");
    }

    if (avformat_write_header(output_ctx, nullptr) < 0)
        throw std::runtime_error("Error occurred when writing header");


    // Read, transcode, and write frames (simplified for clarity)
    AVPacket packet;
    AVFrame* input_frame = av_frame_alloc();
    AVFrame* video_frame = av_frame_alloc();
    AVFrame* audio_frame = av_frame_alloc();
    if (!input_frame || !video_frame || !audio_frame)
        throw std::runtime_error("Failed to allocate frames");

    while (std::shared_ptr<AVPacket> packet = ReadNextPacket()) {
        std::shared_ptr<AVStream> stream = GetStream(packet->stream_index);
        AVCodecParameters *codecpar = stream->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("Found video stream at index %d\n", packet->stream_index);
            AVStream *video_stream = avformat_new_stream(output_ctx, nullptr);
            if (!video_stream)
                throw std::runtime_error("Failed to create video stream");

            AVCodec *video_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
            if (!video_codec)
                throw std::runtime_error("H264 codec not found");

            AVCodecContext* video_codec_ctx = avcodec_alloc_context3(video_codec);
            video_codec_ctx->codec_id = AV_CODEC_ID_H264;
            video_codec_ctx->bit_rate = 0; // Use CRF instead of fixed bitrate
            video_codec_ctx->width = stream->codecpar->width;
            video_codec_ctx->height = stream->codecpar->height;
            video_codec_ctx->time_base = {1, 30}; // 30fps
            video_codec_ctx->framerate = {30, 1};
            video_codec_ctx->gop_size = 12; // Group of pictures
            video_codec_ctx->max_b_frames = 2;
            video_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

            // Set CRF for H.264
            av_opt_set(video_codec_ctx->priv_data, "preset", "slow", 0);
            av_opt_set(video_codec_ctx->priv_data, "crf", "30", 0);

            if (output_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                video_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            if (avcodec_open2(video_codec_ctx, video_codec, nullptr) < 0)
                throw std::runtime_error("Failed to open video codec");

            avcodec_parameters_from_context(video_stream->codecpar, video_codec_ctx);
            video_stream->time_base = video_codec_ctx->time_base;
            //avcodec_free_context(&video_codec_ctx);


            // Video scaler context for frame resizing
            struct SwsContext* sws_ctx = sws_getContext(
                stream->codecpar->width,
                stream->codecpar->height,
                (AVPixelFormat)stream->codecpar->format,
                video_codec_ctx->width,
                video_codec_ctx->height,
                video_codec_ctx->pix_fmt,
                SWS_BILINEAR,
                nullptr,
                nullptr,
                nullptr
            );
            if (!sws_ctx)
                throw std::runtime_error("Failed to create SwsContext");

            // sws_freeContext(sws_ctx);

            // Decode video frame
            if (avcodec_send_packet(stream->codec, packet.get()) < 0)
                continue;

            while (avcodec_receive_frame(stream->codec, input_frame) == 0) {
                // Convert and scale video frame
                av_image_alloc(video_frame->data, video_frame->linesize,
                            video_codec_ctx->width, video_codec_ctx->height,
                            video_codec_ctx->pix_fmt, 32);
                sws_scale(
                    sws_ctx,
                    input_frame->data,
                    input_frame->linesize,
                    0,
                    stream->codecpar->height,
                    video_frame->data,
                    video_frame->linesize
                );

                // Encode video frame
                if (avcodec_send_frame(video_codec_ctx, video_frame) >= 0) {
                    AVPacket out_pkt;
                    av_init_packet(&out_pkt);
                    out_pkt.data = nullptr;
                    out_pkt.size = 0;
                    while (avcodec_receive_packet(video_codec_ctx, &out_pkt) == 0) {
                        av_interleaved_write_frame(output_ctx, &out_pkt);
                        av_packet_unref(&out_pkt);
                    }
                }
                av_frame_unref(video_frame);
            }

        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("Found audio stream at index %d\n", packet->stream_index);
            AVStream* audio_stream = avformat_new_stream(output_ctx, nullptr);
            if (!audio_stream)
                throw std::runtime_error("Failed to create audio stream");

            AVCodec* audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
            if (!audio_codec)
                throw std::runtime_error("AAC codec not found");

            AVCodecContext* audio_codec_ctx = avcodec_alloc_context3(audio_codec);
            audio_codec_ctx->codec_id = AV_CODEC_ID_AAC;
            audio_codec_ctx->bit_rate = 144000; // 144 kbps
            audio_codec_ctx->sample_rate = stream->codecpar->sample_rate;
            audio_codec_ctx->channel_layout = stream->codecpar->channel_layout;
            audio_codec_ctx->channels = av_get_channel_layout_nb_channels(audio_codec_ctx->channel_layout);
            audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
            audio_codec_ctx->time_base = {1, audio_codec_ctx->sample_rate};

            if (output_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                audio_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            if (avcodec_open2(audio_codec_ctx, audio_codec, nullptr) < 0)
                throw std::runtime_error("Failed to open audio codec");

            avcodec_parameters_from_context(audio_stream->codecpar, audio_codec_ctx);
            audio_stream->time_base = audio_codec_ctx->time_base;
            //avcodec_free_context(&audio_codec_ctx);

            // Audio resampler context
            struct SwrContext* swr_ctx = swr_alloc_set_opts(
                nullptr,
                audio_codec_ctx->channel_layout,
                audio_codec_ctx->sample_fmt,
                audio_codec_ctx->sample_rate,
                stream->codecpar->channel_layout,
                (AVSampleFormat)stream->codecpar->format,
                stream->codecpar->sample_rate,
                0,
                nullptr
            );
            if (!swr_ctx || swr_init(swr_ctx) < 0)
                throw std::runtime_error("Failed to create SwrContext");

            //swr_free(&swr_ctx);

            // Decode audio frame
            if (avcodec_send_packet(stream->codec, packet.get()) < 0)
                continue;

            while (avcodec_receive_frame(stream->codec, input_frame) == 0) {
                // Resample audio frame
                int nb_samples = av_rescale_rnd(
                    swr_get_delay(swr_ctx, stream->codecpar->sample_rate) +
                        input_frame->nb_samples,
                    audio_codec_ctx->sample_rate,
                    stream->codecpar->sample_rate,
                    AV_ROUND_UP
                );

                av_frame_make_writable(audio_frame);
                audio_frame->nb_samples = nb_samples;
                audio_frame->channel_layout = audio_codec_ctx->channel_layout;
                audio_frame->format = audio_codec_ctx->sample_fmt;
                audio_frame->sample_rate = audio_codec_ctx->sample_rate;

                swr_convert(
                    swr_ctx,
                    audio_frame->data,
                    audio_frame->nb_samples,
                    (const uint8_t**)input_frame->data,
                    input_frame->nb_samples
                );

                // Encode audio frame
                if (avcodec_send_frame(audio_codec_ctx, audio_frame) >= 0) {
                    AVPacket out_pkt;
                    av_init_packet(&out_pkt);
                    out_pkt.data = nullptr;
                    out_pkt.size = 0;
                    while (avcodec_receive_packet(audio_codec_ctx, &out_pkt) == 0) {
                        av_interleaved_write_frame(output_ctx, &out_pkt);
                        av_packet_unref(&out_pkt);
                    }
                }
                av_frame_unref(audio_frame);
            }
        }
    }

    av_write_trailer(output_ctx);
    av_frame_free(&input_frame);
    av_frame_free(&video_frame);
    av_frame_free(&audio_frame);
    avformat_free_context(output_ctx);
    return true;
}