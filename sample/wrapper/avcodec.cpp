#include "avcodec.h"

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

bool FFAVCodec::SetParameters(const AVCodecParameters *params) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = avcodec_parameters_to_context(context_.get(), params);
    if (ret < 0) {
        return false;
    }
    return true;
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

bool FFAVCodec::Opened() const {
    return opened_.load();
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

void FFAVEncoder::SetFlags(int flags) {
    context_->flags |= flags;
}

bool FFAVEncoder::SetPrivData(const std::string& name, const std::string& val, int search_flags) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = av_opt_set(context_->priv_data, name.c_str(), val.c_str(), search_flags);
    return bool(ret == 0);
}

std::shared_ptr<AVPacket> FFAVEncoder::Encode(std::shared_ptr<AVFrame> frame) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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
