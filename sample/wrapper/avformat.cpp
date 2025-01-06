#include "avformat.h"

FFAVFormat::AVFormatInitPtr FFAVFormat::inited_ = FFAVFormat::AVFormatInitPtr(
    new std::atomic_bool(false),
    [](std::atomic_bool *p) {
        if (p->load()) {
            avformat_network_deinit();
        }
    }
);

bool FFAVFormat::initialize(const std::string& uri, AVFormatContextPtr context) {
    if (!inited_->load()) {
        int ret = avformat_network_init();
        if (ret < 0)
            return false;
        inited_->store(true);
    }

    uri_ = uri;
    context_ = std::move(context);
    return true;
}

int FFAVFormat::toStreamIndex(int stream_id) const {
    for (uint32_t i = 0; i < context_->nb_streams; i++) {
        auto stream = context_->streams[i];
        if (stream->id == stream_id) {
            return stream->index;
        }
    }
    return -1;
}

void FFAVFormat::dumpStreams(int is_output) const {
    av_dump_format(context_.get(), 0, uri_.c_str(), is_output);
}

std::string FFAVFormat::GetURI() const {
    return uri_;
}

uint32_t FFAVFormat::GetStreamNum() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return context_->nb_streams;
}

std::shared_ptr<AVStream> FFAVFormat::GetStream(int stream_index) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (stream_index < 0 || stream_index >= (int)context_->nb_streams) {
        return nullptr;
    }

    AVStream *stream = context_->streams[stream_index];
    return std::shared_ptr<AVStream>(stream, [](AVStream*) {});
}

std::shared_ptr<AVStream> FFAVFormat::GetStreamByID(int stream_id) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return GetStream(toStreamIndex(stream_id));
}

std::shared_ptr<FFAVDemuxer> FFAVDemuxer::Create(const std::string& uri) {
    auto instance = std::shared_ptr<FFAVDemuxer>(new FFAVDemuxer());
    if (!instance->initialize(uri))
        return nullptr;
    return instance;
}

bool FFAVDemuxer::initialize(const std::string& uri) {
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

    return FFAVFormat::initialize(uri, AVFormatContextPtr(
        context,
        [&](AVFormatContext* ctx) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            avformat_close_input(&ctx);
        }
    ));
}

std::shared_ptr<FFAVDecoder> FFAVDemuxer::openDecoder(int stream_index) {
    if (!codecs_.count(stream_index)) {
        std::shared_ptr<AVStream> stream = GetStream(stream_index);
        if (!stream) {
            return nullptr;
        }

        AVCodecParameters *codec_params = stream->codecpar;
        auto codec = FFAVDecoder::Create(codec_params->codec_id);
        if (!codec) {
            return nullptr;
        }

        codec->SetTimeBase(stream->time_base);
        if (!codec->SetParameters(*codec_params)) {
            return nullptr;
        }

        if (!codec->Open()) {
            return nullptr;
        }

        codecs_[stream_index] = codec;
    }
    return codecs_[stream_index];
}

std::shared_ptr<FFAVDecoder> FFAVDemuxer::GetDecoder(int stream_index) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return codecs_.count(stream_index) ? codecs_[stream_index] : nullptr;
}

std::pair<int, std::shared_ptr<AVPacket>> FFAVDemuxer::ReadPacket() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return { AVERROR(ENOMEM), nullptr };
    }

    int ret = av_read_frame(context_.get(), packet);
    if (ret < 0) {
        av_packet_free(&packet);
        return { ret, nullptr };
    }

    auto stream = GetStream(packet->stream_index);
    if (stream) {
        packet->time_base = stream->time_base;
    }

    auto packet_ptr = std::shared_ptr<AVPacket>(packet, [&](AVPacket *p) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        av_packet_unref(p);
        av_packet_free(&p);
    });
    return { 0, packet_ptr };
}

std::pair<int, std::shared_ptr<AVFrame>> FFAVDemuxer::Decode(std::shared_ptr<AVPacket> packet) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto codec = openDecoder(packet->stream_index);
    if (!codec) {
        return { AVERROR(EIO), nullptr };
    }

    return codec->Decode(packet);
}

bool FFAVDemuxer::Seek(int stream_index, int64_t timestamp) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int ret = av_seek_frame(context_.get(), stream_index, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        return false;
    }
    return true;
}

void FFAVDemuxer::DumpStreams() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    FFAVFormat::dumpStreams(0);
}

std::shared_ptr<FFAVMuxer> FFAVMuxer::Create(const std::string& uri, const std::string& mux_fmt) {
    auto instance = std::shared_ptr<FFAVMuxer>(new FFAVMuxer());
    if (!instance->initialize(uri, mux_fmt))
        return nullptr;
    return instance;
}

bool FFAVMuxer::initialize(const std::string& uri, const std::string& mux_fmt) {
    AVFormatContext *context = nullptr;
    const char *filename = uri.empty() ? NULL : uri.c_str();
    const char *format_name = mux_fmt.empty() ? NULL : mux_fmt.c_str();
    int ret = avformat_alloc_output_context2(&context, nullptr, format_name, filename);
    if (ret < 0) {
        std::cerr << "avformat_alloc_output_context2: " << AVError2Str(ret) << std::endl;
        return false;
    }

    return FFAVFormat::initialize(uri, AVFormatContextPtr(
        context,
        [&](AVFormatContext* ctx) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (opened_.load()) {
                if (!(ctx->oformat->flags & AVFMT_NOFILE)) {
                    avio_closep(&ctx->pb);
                }
            }
            avformat_free_context(ctx);
        }
    ));
}

bool FFAVMuxer::openMuxer() {
    if (opened_.load())
        return true;

    if (!(context_->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open2(&context_->pb, uri_.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0) {
            std::cerr << "avio_open2: " << AVError2Str(ret) << std::endl;
            return false;
        }
    }

    opened_.store(true);
    return true;
}

void FFAVMuxer::setEncoderFlags(int stream_index) {
    if (context_->oformat->flags & AVFMT_GLOBALHEADER) {
        auto codec = GetEncoder(stream_index);
        auto codec_ctx = codec->GetContext();
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}

std::shared_ptr<FFAVEncoder> FFAVMuxer::openEncoder(int stream_index) {
    if (!codecs_.count(stream_index))
        return nullptr;

    setEncoderFlags(stream_index);
    auto codec = codecs_[stream_index];
    if (!codec->Open()) {
        return nullptr;
    }
    return codec;
}

std::shared_ptr<FFAVEncoder> FFAVMuxer::GetEncoder(int stream_index) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return codecs_.count(stream_index) ? codecs_[stream_index] : nullptr;
}

std::shared_ptr<AVStream> FFAVMuxer::AddStream(AVCodecID codec_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    AVStream *stream = nullptr;
    if (codec_id != AV_CODEC_ID_NONE) {
        auto codec = FFAVEncoder::Create(codec_id);
        if (!codec) {
            return nullptr;
        }

        stream = avformat_new_stream(context_.get(), codec->GetCodec());
        if (!stream) {
            return nullptr;
        }

        codecs_[stream->index] = codec;
    } else {
        stream = avformat_new_stream(context_.get(), nullptr);
        if (!stream) {
            return nullptr;
        }
    }

    return std::shared_ptr<AVStream>(stream, [](AVStream*) {});
}

bool FFAVMuxer::SetTimeBase(int stream_index, const AVRational& time_base) {
    auto stream = GetStream(stream_index);
    if (!stream)
        return false;

    stream->time_base = time_base;
    auto codec = GetEncoder(stream_index);
    if (codec) {
        codec->SetTimeBase(time_base);
    }
    return true;
}

bool FFAVMuxer::SetParams(int stream_index, const AVCodecParameters& params) {
    auto stream = GetStream(stream_index);
    if (!stream)
        return false;

    auto codec = GetEncoder(stream_index);
    if (codec) {
        if (!codec->SetParameters(params))
            return false;

        int ret = avcodec_parameters_from_context(stream->codecpar, codec->GetContext());
        if (ret < 0) {
            return false;
        }
    } else {
        int ret = avcodec_parameters_copy(stream->codecpar, &params);
        if (ret < 0)
            return false;
    }
    return true;
}

int FFAVMuxer::WriteHeader() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!openMuxer())
        return AVERROR(EIO);

    int ret = avformat_write_header(context_.get(), nullptr);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int FFAVMuxer::WriteTrailer() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!openMuxer())
        return AVERROR(EIO);

    int ret = av_write_trailer(context_.get());
    if (ret < 0)
        return ret;

    return 0;
}

int FFAVMuxer::WritePacket(std::shared_ptr<AVPacket> packet) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!openMuxer())
        return AVERROR(EIO);

    int ret = av_interleaved_write_frame(context_.get(), packet.get());
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int FFAVMuxer::WriteFrame(int stream_index, std::shared_ptr<AVFrame> frame) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!openMuxer())
        return AVERROR(EIO);

    auto codec = openEncoder(stream_index);
    if (!codec)
        return AVERROR(EIO);

    auto [ret, packet] = codec->Encode(frame);
    if (ret < 0)
        return ret;

    packet->stream_index = stream_index;
    return WritePacket(packet);
}

void FFAVMuxer::DumpStreams() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    FFAVFormat::dumpStreams(1);
}
