#include "avcodec.h"

void FFAVCodec::DumpParameters(const AVCodecParameters* params) {
    std::cout << "AVCodecParameters:" << std::endl;
    std::cout << "    codec_type: " << params->codec_type << " (" << av_get_media_type_string(params->codec_type) << ")" << std::endl;
    std::cout << "    codec_id: " << params->codec_id << " (" << avcodec_get_name(params->codec_id) << ")" << std::endl;
    std::cout << "    codec_tag: " << params->codec_tag << std::endl;
    std::cout << "    bit_rate: " << params->bit_rate << std::endl;
    std::cout << "    format: " << params->format << std::endl;
    if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
        std::cout << "    width: " << params->width << std::endl;
        std::cout << "    height: " << params->height << std::endl;
        std::cout << "    sample_aspect_ratio: " << params->sample_aspect_ratio.num << "/" << params->sample_aspect_ratio.den << std::endl;
        std::cout << "    field_order: " << params->field_order << std::endl;
        std::cout << "    color_range: " << params->color_range << std::endl;
        std::cout << "    color_primaries: " << params->color_primaries << std::endl;
        std::cout << "    color_trc: " << params->color_trc << std::endl;
        std::cout << "    color_space: " << params->color_space << std::endl;
        std::cout << "    chroma_location: " << params->chroma_location << std::endl;
        std::cout << "    video_delay: " << params->video_delay << std::endl;
    } else if (params->codec_type == AVMEDIA_TYPE_AUDIO) {
        std::cout << "    sample_rate: " << params->sample_rate << std::endl;
        //std::cout << "    channels: " << params->channels << std::endl;
        //std::cout << "    channel_layout: " << params->channel_layout << std::endl;
        std::cout << "    frame_size: " << params->frame_size << std::endl;
    } else if (params->codec_type == AVMEDIA_TYPE_SUBTITLE) {
        std::cout << "    width: " << params->width << std::endl;
        std::cout << "    height: " << params->height << std::endl;
    }
    std::cout << "    extradata_size: " << params->extradata_size << std::endl;
    if (params->extradata && params->extradata_size > 0) {
        std::cout << "    extradata: ";
        for (int i = 0; i < params->extradata_size; i++) {
            std::cout << std::hex << (int)params->extradata[i] << " ";
        }
        std::cout << std::dec << std::endl;
    }
}

bool FFAVCodec::initialize(const AVCodec *codec) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    AVCodecContext *context = avcodec_alloc_context3(codec);
    if (!context)
        return false;

    codec_ = AVCodecPtr(codec);
    context_ = AVCodecContextPtr(context, [](AVCodecContext* ctx) {
        avcodec_free_context(&ctx);
    });
    return true;
}

AVCodecContext* FFAVCodec::GetContext() const {
    return context_.get();
}

const AVCodec* FFAVCodec::GetCodec() const {
    return codec_.get();
}

std::shared_ptr<AVCodecParameters> FFAVCodec::GetParameters() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    AVCodecParameters *params = avcodec_parameters_alloc();
    int ret = avcodec_parameters_from_context(params, context_.get());
    if (ret < 0) {
        avcodec_parameters_free(&params);
        return nullptr;
    }
    return std::shared_ptr<AVCodecParameters>(params, [](AVCodecParameters *p) {
        avcodec_parameters_free(&p);
    });
}

std::shared_ptr<FFSWScale> FFAVCodec::GetSWScale() const {
    return swscale_;
}

bool FFAVCodec::SetSWScale(int dst_width, int dst_height, AVPixelFormat dst_pix_fmt, int flags) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto params = GetParameters();
    auto swscale = std::make_shared<FFSWScale>(
        params->width, params->height, (AVPixelFormat)params->format,
        dst_width, dst_height, dst_pix_fmt, flags
    );
    if (!swscale->Init())
        return false;

    swscale_ = swscale;
    return true;
}

bool FFAVCodec::Open() {
    if (opened_.load())
        return true;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = avcodec_open2(context_.get(), codec_.get(), nullptr);
    if (ret < 0) {
        return false;
    }

    opened_.store(true);
    return true;
}

std::shared_ptr<FFAVDecoder> FFAVDecoder::Create(AVCodecID id) {
    auto instance = std::shared_ptr<FFAVDecoder>(new FFAVDecoder());
    if (!instance->initialize(id))
        return nullptr;
    return instance;
}

bool FFAVDecoder::initialize(AVCodecID id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const AVCodec *codec = avcodec_find_decoder(id);
    if (!codec)
        return false;

    return FFAVCodec::initialize(codec);
}

bool FFAVDecoder::SetParameters(const AVCodecParameters& params) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = avcodec_parameters_to_context(context_.get(), &params);
    if (ret < 0) {
        return false;
    }
    return true;
}

void FFAVDecoder::SetTimeBase(const AVRational& time_base) {
    /* Inform the decoder about the timebase for the packet timestamps.
     * This is highly recommended, but not mandatory. */
    context_->pkt_timebase = time_base;
}

std::shared_ptr<AVFrame> FFAVDecoder::Decode(std::shared_ptr<AVPacket> packet) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = avcodec_send_packet(context_.get(), packet.get());
    if (ret < 0) {
        return nullptr;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        return nullptr;
    }

    ret = avcodec_receive_frame(context_.get(), frame);
    if (ret < 0) {
        std::string errdesc(AV_ERROR_MAX_STRING_SIZE, '\0');
        av_strerror(ret, errdesc.data(), errdesc.size());
        fprintf(stderr, "avcodec_receive_frame: %s\n", errdesc.c_str());
        av_frame_free(&frame);
        return nullptr;
    }

    auto frame_ptr = std::shared_ptr<AVFrame>(
        frame,
        [](AVFrame *p) {
            av_frame_unref(p);
            av_frame_free(&p);
        }
    );
    if (swscale_) {
        auto params = GetParameters();
        frame_ptr = swscale_->Scale(frame_ptr, 0, params->height, 32);
    }
    return frame_ptr;
}

std::shared_ptr<FFAVEncoder> FFAVEncoder::Create(AVCodecID id) {
    auto instance = std::shared_ptr<FFAVEncoder>(new FFAVEncoder());
    if (!instance->initialize(id))
        return nullptr;
    return instance;
}

bool FFAVEncoder::initialize(AVCodecID id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const AVCodec *codec = avcodec_find_encoder(id);
    if (!codec)
        return false;

    return FFAVCodec::initialize(codec);
}

bool FFAVEncoder::SetParameters(const AVCodecParameters& params) {
    context_->bit_rate = params.bit_rate;
    if (params.codec_type == AVMEDIA_TYPE_VIDEO) {
        context_->width = params.width;
        context_->height = params.height;
        context_->framerate = params.framerate;
        context_->pix_fmt = (AVPixelFormat)params.format;
    } else if (params.codec_type == AVMEDIA_TYPE_AUDIO) {
        context_->sample_fmt = (AVSampleFormat)params.format;
        context_->sample_rate = params.sample_rate;
        int ret = av_channel_layout_copy(&context_->ch_layout, &params.ch_layout);
        if (ret < 0)
            return false;
    }
    return true;
}

void FFAVEncoder::SetTimeBase(const AVRational& time_base) {
    context_->time_base = time_base;
}

void FFAVEncoder::SetSampleAspectRatio(const AVRational& sar) {
    context_->sample_aspect_ratio = sar;
}

void FFAVEncoder::SetGopSize(int gop_size) {
    context_->gop_size = gop_size;
}

void FFAVEncoder::SetMaxBFrames(int max_b_frames) {
    context_->max_b_frames = max_b_frames;
}

void FFAVEncoder::SetFlags(int flags) {
    context_->flags |= flags;
}

bool FFAVEncoder::SetPrivData(const std::string& name, const std::string& val, int search_flags) {
    int ret = av_opt_set(context_->priv_data, name.c_str(), val.c_str(), search_flags);
    return bool(ret == 0);
}

std::shared_ptr<AVPacket> FFAVEncoder::Encode(std::shared_ptr<AVFrame> frame) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = avcodec_send_frame(context_.get(), frame.get());
    if (ret < 0) {
        std::cerr << "avcodec_send_frame: " << AVError2Str(ret) << std::endl;
        return nullptr;
    }

    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return nullptr;
    }

    ret = avcodec_receive_packet(context_.get(), packet);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN)) { // 编码器还没有输出数据包，继续发送帧
        } else if (ret == AVERROR_EOF) { // 编码结束
        }
        std::cerr << "avcodec_receive_packet: " << AVError2Str(ret) << std::endl;
        av_packet_free(&packet);
        return nullptr;
    }

    auto packet_ptr = std::shared_ptr<AVPacket>(
        packet,
        [](AVPacket *p) {
            av_packet_unref(p);
            av_packet_free(&p);
        }
    );

    return packet_ptr;
}
