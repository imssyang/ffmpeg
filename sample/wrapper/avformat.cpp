#include "avformat.h"

bool FFAVStream::initialize(std::shared_ptr<AVStream> stream) {
    stream_ = stream;
    return true;
}

std::shared_ptr<AVStream> FFAVStream::GetStream() const {
    return stream_;
}

std::shared_ptr<FFAVDecodeStream> FFAVDecodeStream::Create(std::shared_ptr<AVStream> stream, bool enable_decode) {
    auto instance = std::shared_ptr<FFAVDecodeStream>(new FFAVDecodeStream());
    if (!instance->initialize(stream, enable_decode))
        return nullptr;
    return instance;
}

bool FFAVDecodeStream::initialize(std::shared_ptr<AVStream> stream, bool enable_decode) {
    if (enable_decode) {
        AVCodecParameters *codecpar = stream->codecpar;
        AVCodecID codec_id = codecpar->codec_id;
        if (codec_id == AV_CODEC_ID_NONE)
            return false;

        auto codec = FFAVDecoder::Create(codec_id);
        if (!codec)
            return false;

        codec->SetTimeBase(stream->time_base);
        if (!codec->SetParameters(*codecpar))
            return false;

        if (!codec->Open()) {
            return false;
        }
    }
    return FFAVStream::initialize(stream);
}

std::shared_ptr<AVFrame> FFAVDecodeStream::ReadFrame(std::shared_ptr<AVPacket> packet) {
    if (packet) {
        if (stream_->index != packet->stream_index) {
            return nullptr;
        }
    }

    auto frame = decoder_->Decode(packet);
    if (!frame) {
        return nullptr;
    }

    frame->time_base = stream_->time_base;
    return frame;
}

bool FFAVDecodeStream::NeedMorePacket() const {
    return decoder_->NeedMorePacket();
}

bool FFAVDecodeStream::FulledBuffer() const {
    return decoder_->FulledBuffer();
}

std::shared_ptr<FFAVEncodeStream> FFAVEncodeStream::Create(std::shared_ptr<AVStream> stream, std::shared_ptr<FFAVEncoder> encoder) {
    auto instance = std::shared_ptr<FFAVEncodeStream>(new FFAVEncodeStream());
    if (!instance->initialize(stream, encoder))
        return nullptr;
    return instance;
}

bool FFAVEncodeStream::initialize(std::shared_ptr<AVStream> stream, std::shared_ptr<FFAVEncoder> encoder) {
    encoder_ = encoder;
    return FFAVStream::initialize(stream);
}

std::shared_ptr<FFAVEncoder> FFAVEncodeStream::GetEncoder() const {
    return encoder_;
}

bool FFAVEncodeStream::SetParameters(const AVCodecParameters& params) {
    if (encoder_) {
        if (!encoder_->SetParameters(params))
            return false;

        int ret = avcodec_parameters_from_context(stream_->codecpar, encoder_->GetContext());
        if (ret < 0) {
            return false;
        }
    } else {
        int ret = avcodec_parameters_copy(stream_->codecpar, &params);
        if (ret < 0)
            return false;
    }
    return true;
}

bool FFAVEncodeStream::SetTimeBase(const AVRational& time_base) {
    stream_->time_base = time_base;
    encoder_->SetTimeBase(time_base);
    return true;
}

bool FFAVEncodeStream::OpenEncoder() {
    if (openencoded_.load())
        return true;

    if (!encoder_->Open())
        return false;

    openencoded_.store(true);
    return true;
}

bool FFAVEncodeStream::Openencoded() const {
    return openencoded_.load();
}

std::shared_ptr<AVPacket> FFAVEncodeStream::ReadPacket(std::shared_ptr<AVFrame> frame) {
    auto packet = encoder_->Encode(frame);
    if (!packet) {
        return nullptr;
    }

    packet->stream_index = stream_->index;
    return packet;
}

bool FFAVEncodeStream::NeedMoreFrame() const {
    return encoder_->NeedMoreFrame();
}

bool FFAVEncodeStream::FulledBuffer() const {
    return encoder_->FulledBuffer();
}

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
        std::cerr << "avformat_open_input(" << uri << "): " << AVError2Str(ret) << std::endl;
        return false;
    }

    ret = avformat_find_stream_info(context, NULL);
    if (ret < 0) {
        std::cerr << "avformat_find_stream_info(" << uri << "): " << AVError2Str(ret) << std::endl;
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

std::shared_ptr<FFAVDecodeStream> FFAVDemuxer::initStream(int stream_index) {
    if (!streams_.count(stream_index)) {
        std::shared_ptr<AVStream> stream = GetStream(stream_index);
        if (!stream) {
            return nullptr;
        }

        auto decode_stream = FFAVDecodeStream::Create(stream, true);
        if (!decode_stream)
            return nullptr;

        streams_[stream_index] = decode_stream;
    }
    return streams_[stream_index];
}

std::shared_ptr<FFAVDecodeStream> FFAVDemuxer::GetDecodeStream(int stream_index) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return streams_.count(stream_index) ? streams_.at(stream_index) : nullptr;
}

std::shared_ptr<AVPacket> FFAVDemuxer::ReadPacket() const {
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

    auto stream = GetStream(packet->stream_index);
    if (stream) {
        packet->time_base = stream->time_base;
    }

    return std::shared_ptr<AVPacket>(packet, [&](AVPacket *p) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        av_packet_unref(p);
        av_packet_free(&p);
    });
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
        std::cerr << "avformat_alloc_output_context2(" << format_name
            << ", " << filename
            << "): " << AVError2Str(ret) << std::endl;
        return false;
    }

    return FFAVFormat::initialize(uri, AVFormatContextPtr(
        context,
        [&](AVFormatContext* ctx) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (openmuxed_.load()) {
                if (!(ctx->oformat->flags & AVFMT_NOFILE)) {
                    avio_closep(&ctx->pb);
                }
            }
            avformat_free_context(ctx);
        }
    ));
}

bool FFAVMuxer::openMuxer() {
    if (openmuxed_.load())
        return true;

    if (!(context_->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open2(&context_->pb, uri_.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0) {
            std::cerr << "avio_open2(" << uri_ << "): " << AVError2Str(ret) << std::endl;
            return false;
        }
    }

    openmuxed_.store(true);
    return true;
}

bool FFAVMuxer::writeHeader() {
    if (headmuxed_.load())
        return true;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!openMuxer())
        return false;

    int ret = avformat_write_header(context_.get(), nullptr);
    if (ret < 0) {
        std::cerr << "avformat_write_header: " << AVError2Str(ret) << std::endl;
        return false;
    }

    headmuxed_.store(true);
    return true;
}

bool FFAVMuxer::writeTrailer() {
    if (trailmuxed_.load())
        return true;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!openMuxer())
        return false;

    int ret = av_write_trailer(context_.get());
    if (ret < 0) {
        std::cerr << "av_write_trailer: " << AVError2Str(ret) << std::endl;
        return false;
    }

    trailmuxed_.store(true);
    return true;
}

std::shared_ptr<FFAVEncodeStream> FFAVMuxer::openEncoder(int stream_index) {
    auto codec_stream = GetEncodeStream(stream_index);
    if (!codec_stream)
        return nullptr;

    if (codec_stream->Openencoded())
        return codec_stream;

    if (context_->oformat->flags & AVFMT_GLOBALHEADER) {
        auto encoder = codec_stream->GetEncoder();
        auto codec_ctx = encoder->GetContext();
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (!codec_stream->OpenEncoder())
        return nullptr;
    return codec_stream;
}

std::shared_ptr<FFAVEncodeStream> FFAVMuxer::GetEncodeStream(int stream_index) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return streams_.count(stream_index) ? streams_.at(stream_index) : nullptr;
}

std::shared_ptr<FFAVEncodeStream> FFAVMuxer::AddStream(AVCodecID codec_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::shared_ptr<FFAVEncodeStream> codec_stream;
    if (codec_id != AV_CODEC_ID_NONE) {
        auto codec = FFAVEncoder::Create(codec_id);
        if (!codec)
            return nullptr;

        auto stream = avformat_new_stream(context_.get(), codec->GetCodec());
        if (!stream)
            return nullptr;

        codec_stream = FFAVEncodeStream::Create(
            std::shared_ptr<AVStream>(stream, [](AVStream*){}),
            codec);
        if (!codec_stream)
            return nullptr;

        streams_[stream->index] = codec_stream;
    } else {
        auto stream = avformat_new_stream(context_.get(), nullptr);
        if (!stream) {
            return nullptr;
        }

        codec_stream = FFAVEncodeStream::Create(
            std::shared_ptr<AVStream>(stream, [](AVStream*){}),
            nullptr);
        if (!codec_stream)
            return nullptr;

        streams_[stream->index] = codec_stream;
    }
    return codec_stream;
}

bool FFAVMuxer::AllowWrite() {
    if (!headmuxed_.load()) {
        if (!writeHeader()) {
            return false;
        }
    }
    return true;
}

bool FFAVMuxer::VerifyMuxer() {
    if (!trailmuxed_.load()) {
        if (!writeTrailer()) {
            return false;
        }
    }
    return true;
}

bool FFAVMuxer::WritePacket(std::shared_ptr<AVPacket> packet) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!openMuxer())
        return false;

    PrintAVPacket(packet.get());

    int ret = av_interleaved_write_frame(context_.get(), packet.get());
    if (ret < 0) {
        std::cerr << "av_interleaved_write_frame: " << AVError2Str(ret) << std::endl;
        return false;
    }

    return true;
}

bool FFAVMuxer::WriteFrame(int stream_index, std::shared_ptr<AVFrame> frame) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!openMuxer())
        return false;

    auto encoder = openEncoder(stream_index);
    if (!encoder)
        return false;

    auto packet = encoder->ReadPacket(frame);
    if (!packet) {
        if (encoder->NeedMoreFrame()) {
            return true;
        }
        return false;
    }

    packet->stream_index = stream_index;
    return WritePacket(packet);
}

void FFAVMuxer::DumpStreams() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    FFAVFormat::dumpStreams(1);
}
