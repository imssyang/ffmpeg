#include <iomanip>
#include "avcodec.h"

bool FFAVCodec::initialize(const AVCodec *codec, int stream_index) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    AVCodecContext *context = avcodec_alloc_context3(codec);
    if (!context)
        return false;

    stream_index_ = stream_index;
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

std::shared_ptr<FFAVDecoder> FFAVDecoder::Create(AVCodecID id, int stream_index) {
    auto instance = std::shared_ptr<FFAVDecoder>(new FFAVDecoder());
    if (!instance->initialize(id, stream_index))
        return nullptr;
    return instance;
}

bool FFAVDecoder::initialize(AVCodecID id, int stream_index) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const AVCodec *codec = avcodec_find_decoder(id);
    if (!codec)
        return false;

    return FFAVCodec::initialize(codec, stream_index);
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

int FFAVDecoder::WritePacket(std::shared_ptr<AVPacket> packet) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return avcodec_send_packet(context_.get(), packet.get());
}

std::pair<int, std::shared_ptr<AVFrame>> FFAVDecoder::ReadFrame() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        return { AVERROR(ENOMEM), nullptr };
    }

    int ret = avcodec_receive_frame(context_.get(), frame);
    if (ret < 0) {
        std::cerr << "avcodec_receive_frame: " << AVError2Str(ret) << std::endl;
        av_frame_free(&frame);
        return { ret, nullptr };
    }

    if (frame->pts == AV_NOPTS_VALUE) {
        frame->pts = frame->best_effort_timestamp;
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
    return { 0, frame_ptr };
}

std::pair<int, std::shared_ptr<AVFrame>> FFAVDecoder::Decode(std::shared_ptr<AVPacket> packet) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (packet) {
        int ret = avcodec_send_packet(context_.get(), packet.get());
        if (ret < 0) {
            if (ret != AVERROR(EAGAIN)) {
                return { ret, nullptr };
            }
        }
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        return { AVERROR(ENOMEM), nullptr };
    }

    int ret = avcodec_receive_frame(context_.get(), frame);
    if (ret < 0) {
        std::cerr << "avcodec_receive_frame: " << AVError2Str(ret) << std::endl;
        av_frame_free(&frame);
        return { ret, nullptr };
    }

    if (frame->pts == AV_NOPTS_VALUE) {
        frame->pts = frame->best_effort_timestamp;
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
    return { 0, frame_ptr };
}

std::shared_ptr<FFAVEncoder> FFAVEncoder::Create(AVCodecID id, int stream_index) {
    auto instance = std::shared_ptr<FFAVEncoder>(new FFAVEncoder());
    if (!instance->initialize(id, stream_index))
        return nullptr;
    return instance;
}

bool FFAVEncoder::initialize(AVCodecID id, int stream_index) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const AVCodec *codec = avcodec_find_encoder(id);
    if (!codec)
        return false;

    return FFAVCodec::initialize(codec, stream_index);
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

std::pair<int, std::shared_ptr<AVPacket>> FFAVEncoder::Encode(std::shared_ptr<AVFrame> frame) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    bool send_ok = false;
    int ret = avcodec_send_frame(context_.get(), frame.get());
    if (ret < 0) {
        if (ret != AVERROR(EAGAIN)) { // wait read output
            return { ret, nullptr};
        }
    } else {
        send_ok = true;
    }

    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return { AVERROR(ENOMEM), nullptr };
    }

    ret = avcodec_receive_packet(context_.get(), packet);
    if (ret < 0) {
        av_packet_free(&packet);
        return { ret, nullptr };
    }

    if (!send_ok) {
        
    }

    auto packet_ptr = std::shared_ptr<AVPacket>(
        packet,
        [](AVPacket *p) {
            av_packet_unref(p);
            av_packet_free(&p);
        }
    );

    return { 0, packet_ptr };
}

void PrintAVPacket(const AVPacket* packet) {
    if (!packet) {
        return;
    }

    std::cout << "packet"
        << " index:" << packet->stream_index
        << " dts:" << packet->dts
        << " pts:" << packet->pts
        << " duration:" << packet->duration
        << " time_base:" << packet->time_base.num << "/" << packet->time_base.den
        << " size:" << packet->size
        << " pos:" << packet->pos;

    std::cout << " flags:";
    if (packet->flags & AV_PKT_FLAG_KEY) std::cout << "K";
    if (packet->flags & AV_PKT_FLAG_CORRUPT) std::cout << "C";
    if (packet->flags & AV_PKT_FLAG_DISCARD) std::cout << "D";
    if (packet->flags & AV_PKT_FLAG_TRUSTED) std::cout << "T";
    if (packet->flags & AV_PKT_FLAG_DISPOSABLE) std::cout << "P";

    std::cout << " side_data:";
    for (int i = 0; i < packet->side_data_elems; i++) {
        const AVPacketSideData& sd = packet->side_data[i];
        std::cout << av_packet_side_data_name(sd.type) << "@" << sd.size << " ";
    }

    if (packet->size > 0) {
        std::cout << " head:";
        for (int i = 0; i < std::min(packet->size, 8); ++i) {
            std::cout << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << static_cast<int>(packet->data[i]) << " ";
        }
        std::cout << std::dec;
    }
    std::cout << std::endl;
}

void PrintAVCodecParameters(const AVCodecParameters* params) {
    if (!params)
        return;

    std::cout << av_get_media_type_string(params->codec_type)
        << " " << avcodec_get_name(params->codec_id)
        << "/" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << params->codec_tag << std::dec
        << " bit_rate:" << params->bit_rate;

    if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
        std::cout << " pix_fmt:" << av_get_pix_fmt_name(static_cast<AVPixelFormat>(params->format))
            << " width:" << params->width
            << " height:" << params->height
            << " SAR:" << params->sample_aspect_ratio.num << "/" << params->sample_aspect_ratio.den
            << " framerate:" << params->framerate.num << "/" << params->framerate.den
            << " field_order:" << params->field_order
            << " color_range:" << av_color_range_name(params->color_range)
            << " color_primaries:" << av_color_primaries_name(params->color_primaries)
            << " color_trc:" << av_color_transfer_name(params->color_trc)
            << " color_space:" << av_color_space_name(params->color_space)
            << " chroma_location:" << av_chroma_location_name(params->chroma_location)
            << " video_delay:" << params->video_delay;
    } else if (params->codec_type == AVMEDIA_TYPE_AUDIO) {
        std::cout << " sample_fmt:" << av_get_sample_fmt_name(static_cast<AVSampleFormat>(params->format))
            << " sample_rate:" << params->sample_rate
            << " ch_layout:" << AVChannelLayoutStr(&params->ch_layout)
            << " size:" << params->frame_size;
    } else if (params->codec_type == AVMEDIA_TYPE_SUBTITLE) {
        std::cout << " width:" << params->width
            << " height:" << params->height;
    }
    if (params->extradata && params->extradata_size > 0) {
        std::cout << " extradata:";
        for (int i = 0; i < params->extradata_size; i++) {
            std::cout << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << (int)params->extradata[i] << " ";
        }
        std::cout << std::dec;
    }
    std::cout << std::endl;
}