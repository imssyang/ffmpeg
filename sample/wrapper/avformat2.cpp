#include "avformat.h"

std::string FFAVBaseIO::GetURI() const {
    return uri_;
}

FFAVDirection FFAVBaseIO::GetDirection() const {
    return direct_;
}

void FFAVBaseIO::DumpStreams() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    av_dump_format(context_.get(), 0, uri_.c_str(), 0);
}

int FFAVBaseIO::GetNumOfStreams() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return context_->nb_streams;
}

std::shared_ptr<AVStream> FFAVBaseIO::GetStream(int stream_index) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (stream_index < 0 || stream_index >= (int)context_->nb_streams) {
        return nullptr;
    }

    return std::shared_ptr<AVStream>(
        context_->streams[stream_index],
        [](AVStream*) {}
    );
}

std::shared_ptr<AVStream> FFAVBaseIO::GetStreamByID(int stream_id) const {
    int stream_index = getStreamIndex(stream_id);
    return GetStream(stream_index);
}

std::shared_ptr<AVFrame> FFAVBaseIO::Decode(int stream_index, std::shared_ptr<AVPacket> packet) {
    auto codec = getCodec(stream_index);
    if (!codec)
        return nullptr;
    return codec->Decode(packet);
}

std::shared_ptr<AVPacket> FFAVBaseIO::Encode(int stream_index, std::shared_ptr<AVFrame> frame) {
    auto codec = getCodec(stream_index);
    if (!codec)
        return nullptr;
    auto packet = codec->Encode(frame);
    if (!packet)
        return nullptr;
    packet->stream_index = stream_index;
    return packet;
}

bool FFAVBaseIO::SetSWScale(
    int stream_index, int dst_width, int dst_height,
    AVPixelFormat dst_pix_fmt, int flags) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto codec = getCodec(stream_index);
    if (!codec)
        return false;
    return codec->SetSWScale(dst_width, dst_height, dst_pix_fmt, flags);
}

int FFAVBaseIO::getStreamIndex(int stream_id) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (uint32_t i = 0; i < context_->nb_streams; i++) {
        auto stream = context_->streams[i];
        if (stream->id == stream_id) {
            return stream->index;
        }
    }
    return -1;
}

std::shared_ptr<FFAVInput> FFAVInput::Create(const std::string& uri) {
    auto instance = std::shared_ptr<FFAVInput>(new FFAVInput());
    if (!instance->initialize(uri))
        return nullptr;
    return instance;
}

bool FFAVInput::initialize(const std::string& uri) {
    AVFormatContext *context = nullptr;
    int ret = avformat_open_input(&context, uri.c_str(), NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input file '%s'\n", uri.c_str());
        return false;
    }

    ret = avformat_find_stream_info(context, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&context);
        return false;
    }

    uri_ = uri;
    direct_ = FFAV_DIRECTION_INPUT;
    context_ = AVFormatContextPtr(
        context,
        [&](AVFormatContext* ctx) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            avformat_close_input(&ctx);
        }
    );
    return true;
}

std::shared_ptr<FFAVCodec> FFAVInput::getCodec(int stream_index) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!codecs_.count(stream_index)) {
        std::shared_ptr<AVStream> stream = GetStream(stream_index);
        if (!stream) {
            return nullptr;
        }

        AVCodecParameters *codec_params = stream->codecpar;
        auto codec = FFAVCodec::Create(codec_params->codec_id);
        if (!codec) {
            return nullptr;
        }

        if (!codec->SetParameters(codec_params)) {
            return nullptr;
        }

        if (!codec->Open()) {
            return nullptr;
        }

        codecs_[stream_index] = codec;
    }
    return codecs_[stream_index];
}

std::shared_ptr<AVPacket> FFAVInput::ReadPacket() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return nullptr;
    }

    int ret = av_read_frame(context_.get(), packet);
    if (ret < 0) {
        av_packet_free(&packet);
        return nullptr;
    }

    return std::shared_ptr<AVPacket>(packet, [&](AVPacket* p) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        av_packet_unref(p);
        av_packet_free(&p);
    });
}

std::shared_ptr<AVFrame> FFAVInput::ReadFrame() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::shared_ptr<AVPacket> packet = ReadPacket();
    if (!packet) {
        return nullptr;
    }

    std::shared_ptr<FFAVCodec> codec = getCodec(packet->stream_index);
    if (!codec) {
        return nullptr;
    }

    std::shared_ptr<AVFrame> frame = codec->Decode(packet);
    if (!frame) {
        return nullptr;
    }

    if (frame->pts == AV_NOPTS_VALUE) {
        frame->pts = frame->best_effort_timestamp;
    }
    return frame;
}

bool FFAVInput::Seek(int stream_index, int64_t timestamp) {
    int ret = av_seek_frame(context_.get(), stream_index, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        return false;
    }
    return true;
}

std::shared_ptr<FFAVOutput> FFAVOutput::Create(const std::string& uri, const std::string& mux_fmt) {
    auto instance = std::shared_ptr<FFAVOutput>(new FFAVOutput());
    if (!instance->initialize(uri, mux_fmt))
        return nullptr;
    return instance;
}

std::shared_ptr<AVStream> FFAVOutput::AddVideo(
    AVCodecID codec_id,
    AVPixelFormat pix_fmt,
    const AVRational& time_base,
    const AVRational& framerate,
    int64_t bit_rate,
    int width,
    int height,
    int gop_size,
    int max_b_frames,
    int flags,
    const std::string& crf,
    const std::string& preset
) {
    auto codec = FFAVCodec::Create(codec_id);
    if (!codec) {
        return nullptr;
    }

    codec->SetCodecType(AVMEDIA_TYPE_VIDEO);
    codec->SetPixFmt(pix_fmt);
    codec->SetTimeBase(time_base);
    codec->SetFrameRate(framerate);
    codec->SetBitRate(bit_rate);
    codec->SetWidth(width);
    codec->SetHeight(height);
    codec->SetGopSize(gop_size);
    codec->SetMaxBFrames(max_b_frames);
    codec->SetFlags(flags);
    codec->SetPrivData("preset", preset.c_str(), 0);
    codec->SetPrivData("crf", crf.c_str(), 0);
    if (!codec->Open()) {
        return nullptr;
    }

    const AVCodec* codec_cdc = codec->GetCodec();
    const AVCodecContext *codec_ctx = codec->GetContext();
    AVStream *stream = avformat_new_stream(context_.get(), codec_cdc);
    if (!stream) {
        return nullptr;
    }

    int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    if (ret < 0) {
        return nullptr;
    }

    codecs_[stream->index] = codec;
    stream->time_base = codec_ctx->time_base;
    return std::shared_ptr<AVStream>(stream, [](AVStream*) {});
}

std::shared_ptr<AVStream> FFAVOutput::AddAudio(
    AVCodecID codec_id,
    AVSampleFormat sample_fmt,
    const AVChannelLayout& ch_layout,
    const AVRational& time_base,
    int64_t bit_rate,
    int sample_rate,
    int flags
) {
    auto codec = FFAVCodec::Create(codec_id);
    if (!codec) {
        return nullptr;
    }

    codec->SetCodecType(AVMEDIA_TYPE_AUDIO);
    codec->SetTimeBase(time_base);
    codec->SetSampleFormat(sample_fmt);
    codec->SetChannelLayout(ch_layout);
    codec->SetBitRate(bit_rate);
    codec->SetSampleRate(sample_rate);
    codec->SetFlags(flags);
    if (!codec->Open()) {
        return nullptr;
    }

    const AVCodec* codec_cdc = codec->GetCodec();
    const AVCodecContext *codec_ctx = codec->GetContext();
    AVStream *stream = avformat_new_stream(context_.get(), codec_cdc);
    if (!stream) {
        return nullptr;
    }

    int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    if (ret < 0) {
        return nullptr;
    }

    codecs_[stream->index] = codec;
    stream->time_base = codec_ctx->time_base;
    return std::shared_ptr<AVStream>(stream, [](AVStream*) {});
}

bool FFAVOutput::WriteHeader() {
    if (!open()) return false;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = avformat_write_header(context_.get(), nullptr);
    if (ret < 0) {
        avio_close(context_->pb);
        return false;
    }

    opened_.store(true);
    return true;
}

bool FFAVOutput::WriteTrailer() {
    if (!open()) return false;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = av_write_trailer(context_.get());
    if (ret < 0)
        return false;
    return true;
}

bool FFAVOutput::WritePacket(int stream_index, std::shared_ptr<AVPacket> packet) {
    if (!open()) return false;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    packet->stream_index = stream_index;
    int ret = av_interleaved_write_frame(context_.get(), packet.get());
    if (ret < 0) {
        return false;
    }
    return true;
}

bool FFAVOutput::WriteFrame(int stream_index, std::shared_ptr<AVFrame> frame) {
    if (!open()) return false;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::shared_ptr<FFAVCodec> codec = getCodec(stream_index);
    if (!codec)
        return false;

    std::shared_ptr<AVPacket> packet = codec->Encode(frame);
    if (!packet)
        return false;

    return WritePacket(stream_index, packet);
}

bool FFAVOutput::initialize(const std::string& uri, const std::string& mux_fmt) {
    AVFormatContext *context = nullptr;
    int ret = avformat_alloc_output_context2(&context, nullptr, mux_fmt.c_str(), uri_.c_str());
    if (ret < 0) {
        return false;
    }

    uri_ = uri;
    direct_ = FFAV_DIRECTION_OUTPUT;
    context_ = AVFormatContextPtr(
        context,
        [&](AVFormatContext* ctx) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (opened_.load())
                avio_close(ctx->pb);
            avformat_free_context(ctx);
        }
    );
    return true;
}

bool FFAVOutput::open() {
    if (opened_.load())
        return true;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = avio_open(&context_->pb, uri_.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        return false;
    }
    opened_.store(true);
    return true;
}

std::shared_ptr<FFAVCodec> FFAVOutput::getCodec(int stream_index) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!codecs_.count(stream_index)) {
        return nullptr;
    }
    return codecs_[stream_index];
}

std::atomic_bool FFAVFormat::inited_{false};

std::shared_ptr<FFAVFormat> FFAVFormat::Create(
    const std::string& uri, const std::string& mux_fmt, FFAVDirection direct) {
    auto instance = std::shared_ptr<FFAVFormat>(new FFAVFormat());
    if (!instance->initialize(uri, mux_fmt, direct))
        return nullptr;
    return instance;
}

void FFAVFormat::Destory() {
    if (inited_.load()) {
        avformat_network_deinit();
    }
}

bool FFAVFormat::initialize(const std::string& uri, const std::string& mux_fmt, FFAVDirection direct) {
    if (!inited_.load()) {
        int ret = avformat_network_init();
        if (ret < 0)
            return false;
        inited_.store(true);
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (direct == FFAV_DIRECTION_INPUT) {
        auto input = FFAVInput::initialize(uri);
        if (!input)
            return false;
    } else if (direct == FFAV_DIRECTION_OUTPUT) {
        auto output = FFAVOutput::initialize(uri, mux_fmt);
        if (!output)
            return false;
    } else {
        return false;
    }
    return true;
}

std::shared_ptr<FFAVCodec> FFAVFormat::getCodec(int stream_index) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (direct_ == FFAV_DIRECTION_INPUT) {
        return FFAVInput::getCodec(stream_index);
    } else {
        return FFAVOutput::getCodec(stream_index);
    }
}


/*
bool FFAVFormat::SaveFormat(const std::string& url, AVCodecID video_codec_id, AVCodecID audio_codec_id) {
    if (!context_)
        return false;


    if (audio_codec_id != AV_CODEC_ID_NONE) {
        std::shared_ptr<FFAVCodec> audio_codec = FFAVCodec::Create(audio_codec_id);
    }

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
    //AVPacket packet;
    //AVFrame* input_frame = av_frame_alloc();
    //AVFrame* video_frame = av_frame_alloc();
    //AVFrame* audio_frame = av_frame_alloc();
    //if (!input_frame || !video_frame || !audio_frame)
    //    throw std::runtime_error("Failed to allocate frames");

    while (std::shared_ptr<AVPacket> packet = ReadNextPacket()) {
        std::shared_ptr<AVStream> stream = GetStream(packet->stream_index);
        AVCodecParameters *codecpar = stream->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("Found video stream at index %d\n", packet->stream_index);
            AVStream *video_stream = avformat_new_stream(output_ctx, nullptr);
            if (!video_stream)
                throw std::runtime_error("Failed to create video stream");

            if (video_codec_id != AV_CODEC_ID_NONE && !avcodecs_.count(video_codec_id)) {
                std::shared_ptr<FFAVCodec> video_codec = FFAVCodec::Create(video_codec_id);
                if (video_codec) {
                    video_codec->SetBitRate(0); // Use CRF instead of fixed bitrate
                    video_codec->SetWidth(stream->codecpar->width);
                    video_codec->SetHeight(stream->codecpar->height);
                    video_codec->SetTimeBase({1, 30}); // 30fps
                    video_codec->SetFrameRate({30, 1});
                    video_codec->SetGopSize(12); // Group of pictures
                    video_codec->SetMaxBFrames(2);
                    video_codec->SetPixFmt(AV_PIX_FMT_YUV420P);
                    video_codec->SetPrivData("preset", "slow", 0);
                    video_codec->SetPrivData("crf", "30", 0);
                    if (output_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                        video_codec->SetFlags(AV_CODEC_FLAG_GLOBAL_HEADER);
                    video_codec->SetSWScale();
                    if (!video_codec->Open()) {
                        video_codec = nullptr;
                    }
                }
                avcodecs_[video_codec_id] = video_codec;
            }

            std::shared_ptr<FFAVCodec> video_codec = avcodecs_[video_codec_id];
            if (video_codec) {
                while (std::shared_ptr<AVFrame> frame = video_codec->Decode(packet)) {

                }
            }

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
            struct SwrContext* swr_ctx = swr_alloc_set_opts2(
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
*/