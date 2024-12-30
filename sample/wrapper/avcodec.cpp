#include "avcodec.h"

std::shared_ptr<FFAVCodec> FFAVCodec::Create(AVCodecID id) {
    auto instance = std::shared_ptr<FFAVCodec>(new FFAVCodec(id));
    if (!instance->initialize())
        return nullptr;
    return instance;
}

FFAVCodec::FFAVCodec(AVCodecID id) : id_(id) {
}

bool FFAVCodec::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    const AVCodec *codec = avcodec_find_encoder(id_);
    if (!codec)
        return false;

    AVCodecContext *context = avcodec_alloc_context3(codec);
    if (!context)
        return false;

    codec_ = AVCodecPtr(codec);
    context_->codec_id = id_;
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

std::shared_ptr<FFSWScale> FFAVCodec::GetSWScale() const {
    return swscale_;
}

std::shared_ptr<AVCodecParameters> FFAVCodec::GetParameters() const {
    std::lock_guard<std::mutex> lock(mutex_);
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

bool FFAVCodec::SetParameters(const AVCodecParameters *params) {
    std::lock_guard<std::mutex> lock(mutex_);
    int ret = avcodec_parameters_to_context(context_.get(), params);
    if (ret < 0) {
        return false;
    }
    return true;
}

void FFAVCodec::SetCodecType(AVMediaType codec_type) {
    context_->codec_type = codec_type;
}

void FFAVCodec::SetBitRate(int64_t bit_rate) {
    context_->bit_rate = bit_rate;
}

void FFAVCodec::SetWidth(int width) {
    context_->width = width;
}

void FFAVCodec::SetHeight(int height) {
    context_->height = height;
}

void FFAVCodec::SetTimeBase(const AVRational& time_base) {
    std::lock_guard<std::mutex> lock(mutex_);
    context_->time_base = time_base;
}

void FFAVCodec::SetFrameRate(const AVRational& framerate) {
    std::lock_guard<std::mutex> lock(mutex_);
    context_->framerate = framerate;
}

void FFAVCodec::SetGopSize(int gop_size) {
    context_->gop_size = gop_size;
}

void FFAVCodec::SetMaxBFrames(int max_b_frames) {
    context_->max_b_frames = max_b_frames;
}

void FFAVCodec::SetPixFmt(AVPixelFormat pix_fmt) {
    context_->pix_fmt = pix_fmt;
}

void FFAVCodec::SetSampleRate(int sample_rate) {
    context_->sample_rate = sample_rate;
}

void FFAVCodec::SetSampleFormat(AVSampleFormat sample_fmt) {
    context_->sample_fmt = sample_fmt;
}

void FFAVCodec::SetChannelLayout(const AVChannelLayout& ch_layout) {
    std::lock_guard<std::mutex> lock(mutex_);
    context_->ch_layout = ch_layout;
}

void FFAVCodec::SetFlags(int flags) {
    context_->flags |= flags;
}

bool FFAVCodec::SetPrivData(const std::string& name, const std::string& val, int search_flags) {
    std::lock_guard<std::mutex> lock(mutex_);
    int ret = av_opt_set(context_->priv_data, name.c_str(), val.c_str(), search_flags);
    return bool(ret == 0);
}

bool FFAVCodec::Open() {
    if (opened_.load())
        return true;

    std::lock_guard<std::mutex> lock(mutex_);
    int ret = avcodec_open2(context_.get(), codec_.get(), nullptr);
    if (ret < 0) {
        return false;
    }

    opened_.store(true);
    return true;
}

bool FFAVCodec::SetSWScale(int dst_width, int dst_height, AVPixelFormat dst_pix_fmt, int flags) {
    if (!opened_.load())
        return false;

    std::lock_guard<std::mutex> lock(mutex_);
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

std::shared_ptr<AVFrame> FFAVCodec::Decode(std::shared_ptr<AVPacket> packet) {
    if (!opened_.load())
        return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);
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

std::shared_ptr<AVPacket> FFAVCodec::Encode(std::shared_ptr<AVFrame> frame) {
    if (!opened_.load())
        return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);
    int ret = avcodec_send_frame(context_.get(), frame.get());
    if (ret < 0) {
        return nullptr;
    }

    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return nullptr;
    }

    ret = avcodec_receive_packet(context_.get(), packet);
    if (ret < 0) {
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

