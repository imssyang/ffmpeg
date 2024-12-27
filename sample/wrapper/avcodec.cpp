#include "avcodec.h"

std::shared_ptr<FFAVCodec> FFAVCodec::Create(enum AVCodecID id) {
    auto instance = std::make_shared<FFAVCodec>(id);
    if (!instance->initialize())
        return nullptr;
    return instance;
}

FFAVCodec::FFAVCodec(enum AVCodecID id) : id_(id) {
}

bool FFAVCodec::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    AVCodec *codec = avcodec_find_encoder(id_);
    if (!codec)
        return false;

    AVCodecContext *context = avcodec_alloc_context3(codec);
    if (!context)
        return false;

    context_ = AVCodecContextPtr(context, [](AVCodecContext* ctx) {
        avcodec_free_context(&ctx);
    });
    return true;
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
    context_->time_base = time_base;
}

void FFAVCodec::SetFrameRate(const AVRational& framerate) {
    context_->framerate = framerate;
}

void FFAVCodec::SetGopSize(int gop_size) {
    context_->gop_size = gop_size;
}

void FFAVCodec::SetMaxBFrames(int max_b_frames) {
    context_->max_b_frames = max_b_frames;
}

void FFAVCodec::SetPixFmt(enum AVPixelFormat pix_fmt) {
    context_->pix_fmt = pix_fmt;
}

void FFAVCodec::SetSampleRate(int sample_rate) {
    context_->sample_rate = sample_rate;
}

void FFAVCodec::SetSampleFormat(enum AVSampleFormat sample_fmt) {
    context_->sample_fmt = sample_fmt;
}

void FFAVCodec::SetChannelLayout(uint64_t channel_layout) {
    context_->channel_layout = channel_layout;
    context_->channels = av_get_channel_layout_nb_channels(channel_layout);
}

void FFAVCodec::SetFlags(int flags) {
    context_->flags |= flags;
}

bool FFAVCodec::SetPrivData(const std::string& name, const std::string& val, int search_flags) {
    av_opt_set(context_->priv_data, name.c_str(), val.c_str(), search_flags);
}

bool FFAVCodec::Open(AVStream *stream) {
    if (avcodec_open2(context_.get(), context_->codec, nullptr) < 0)
        return false;

    avcodec_parameters_from_context(stream->codecpar, context_.get());
    stream->time_base = context_->time_base;
    return true;
}
