#include "avformat.h"

std::shared_ptr<FFAVStream> FFAVStream::Create(
    std::shared_ptr<AVFormatContext> context,
    std::shared_ptr<AVStream> stream) {
    auto instance = std::shared_ptr<FFAVStream>(new FFAVStream());
    if (!instance->initialize(context, stream))
        return nullptr;
    return instance;
}

bool FFAVStream::initialize(
    std::shared_ptr<AVFormatContext> context,
    std::shared_ptr<AVStream> stream) {
    context_ = context;
    stream_ = stream;
    return true;
}

AVRational FFAVStream::correctTimeBase(const AVRational& time_base) {
    if (context_->oformat) {
        if (strcmp(context_->oformat->name, "flv") == 0) {
            return {1, 1000};
        }
    }
    return time_base;
}

std::shared_ptr<AVPacket> FFAVStream::transformPacket(std::shared_ptr<AVPacket> packet) {
    if (!context_->oformat)
        return nullptr;

    std::shared_ptr<AVPacket> mux_packet = std::shared_ptr<AVPacket>(
        av_packet_clone(packet.get()),
        [&](AVPacket *p) {
            av_packet_unref(p);
            av_packet_free(&p);
        }
    );

    mux_packet->pts = av_rescale_q_rnd(packet->pts, packet->time_base, stream_->time_base,
        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    mux_packet->dts = av_rescale_q_rnd(packet->dts, packet->time_base, stream_->time_base,
        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    mux_packet->duration = av_rescale_q(packet->duration, packet->time_base, stream_->time_base);
    mux_packet->time_base = stream_->time_base;
    mux_packet->pos = -1;
    return mux_packet;
}

std::shared_ptr<AVFormatContext> FFAVStream::GetContext() const {
    return context_;
}

std::shared_ptr<AVStream> FFAVStream::GetStream() const {
    return stream_;
}

std::shared_ptr<AVCodecParameters> FFAVStream::GetParameters() const {
    return std::shared_ptr<AVCodecParameters>(stream_->codecpar, [](auto){});
}

int FFAVStream::GetIndex() const {
    return stream_->index;
}

std::string FFAVStream::GetMetadata(const std::string& metakey) {
    if (!context_->iformat)
        return {};

    AVDictionaryEntry *entry = av_dict_get(stream_->metadata, metakey.c_str(), nullptr, AV_DICT_IGNORE_SUFFIX);
    if (!entry)
        return {};

    return entry->value;
}

bool FFAVStream::SetMetadata(const std::unordered_map<std::string, std::string>& metadata) {
    if (!context_->oformat)
        return false;

    for (const auto& [key, value] : metadata) {
        int ret = av_dict_set(&stream_->metadata, key.c_str(), value.c_str(), 0);
        if (ret < 0)
            return false;
    }
    return true;
}

bool FFAVStream::SetParameters(const AVCodecParameters& params) {
    if (!context_->oformat)
        return false;

    int ret = avcodec_parameters_copy(stream_->codecpar, &params);
    if (ret < 0)
        return false;

    stream_->codecpar->codec_tag = 0;
    return true;
}

bool FFAVStream::SetTimeBase(const AVRational& time_base) {
    if (!context_->oformat)
        return false;

    stream_->time_base = correctTimeBase(time_base);
    return true;
}

std::shared_ptr<FFAVDecodeStream> FFAVDecodeStream::Create(
    std::shared_ptr<AVFormatContext> context,
    std::shared_ptr<AVStream> stream) {
    auto instance = std::shared_ptr<FFAVDecodeStream>(new FFAVDecodeStream());
    if (!instance->initialize(context, stream))
        return nullptr;
    return instance;
}

bool FFAVDecodeStream::initialize(
    std::shared_ptr<AVFormatContext> context,
    std::shared_ptr<AVStream> stream) {
    return FFAVStream::initialize(context, stream);
}

bool FFAVDecodeStream::initDecoder() {
    if (decoder_)
        return true;

    AVCodecParameters *codecpar = stream_->codecpar;
    AVCodecID codec_id = codecpar->codec_id;
    if (codec_id == AV_CODEC_ID_NONE)
        return false;

    auto decoder = FFAVDecoder::Create(codec_id);
    if (!decoder)
        return false;

    decoder->SetTimeBase(stream_->time_base);
    if (!decoder->SetParameters(*stream_->codecpar))
        return false;

    if (!decoder->Open())
        return false;

    decoder_ = decoder;
    return true;
}

std::shared_ptr<FFAVDecoder> FFAVDecodeStream::GetDecoder() const {
    return decoder_;
}

std::shared_ptr<AVFrame> FFAVDecodeStream::ReadFrame(std::shared_ptr<AVPacket> packet) {
    if (packet) {
        if (stream_->index != packet->stream_index) {
            return nullptr;
        }
    }

    if (!initDecoder())
        return nullptr;

    auto frame = decoder_->Decode(packet);
    if (!frame) {
        return nullptr;
    }

    frame->time_base = stream_->time_base;
    return frame;
}

std::shared_ptr<FFAVEncodeStream> FFAVEncodeStream::Create(
    std::shared_ptr<AVFormatContext> context,
    std::shared_ptr<AVStream> stream,
    std::shared_ptr<FFAVEncoder> encoder) {
    auto instance = std::shared_ptr<FFAVEncodeStream>(new FFAVEncodeStream());
    if (!instance->initialize(context, stream, encoder))
        return nullptr;
    return instance;
}

bool FFAVEncodeStream::initialize(
    std::shared_ptr<AVFormatContext> context,
    std::shared_ptr<AVStream> stream,
    std::shared_ptr<FFAVEncoder> encoder) {
    encoder_ = encoder;
    return FFAVStream::initialize(context, stream);
}

bool FFAVEncodeStream::openEncoder() {
    if (!encoder_)
        return false;

    if (openencoded_.load())
        return true;

    if (context_->oformat->flags & AVFMT_GLOBALHEADER) {
        auto codec_ctx = encoder_->GetContext();
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (!encoder_->Open())
        return false;

    openencoded_.store(true);
    return true;
}

std::shared_ptr<FFAVEncoder> FFAVEncodeStream::GetEncoder() const {
    return encoder_;
}

bool FFAVEncodeStream::SetParameters(const AVCodecParameters& params) {
    if (!encoder_->SetParameters(params))
        return false;

    int ret = avcodec_parameters_from_context(stream_->codecpar, encoder_->GetContext());
    if (ret < 0)
        return false;

    return true;
}

bool FFAVEncodeStream::SetTimeBase(const AVRational& time_base) {
    auto real_time_base = correctTimeBase(time_base);
    if (encoder_)
        encoder_->SetTimeBase(real_time_base);
    return FFAVStream::SetTimeBase(real_time_base);
}

std::shared_ptr<AVPacket> FFAVEncodeStream::ReadPacket(std::shared_ptr<AVFrame> frame) {
    auto encoder = openEncoder();
    if (!encoder)
        return nullptr;

    auto packet = encoder_->Encode(frame);
    if (!packet)
        return nullptr;

    packet->stream_index = stream_->index;
    return packet;
}

FFAVFormat::AVFormatInitPtr FFAVFormat::inited_ = FFAVFormat::AVFormatInitPtr(
    new std::atomic_bool(false),
    [](std::atomic_bool *p) {
        if (p->load()) {
            avformat_network_deinit();
        }
    }
);

bool FFAVFormat::initialize(const std::string& uri, std::shared_ptr<AVFormatContext> context) {
    if (!inited_->load()) {
        int ret = avformat_network_init();
        if (ret < 0)
            return false;
        inited_->store(true);
    }

    uri_ = uri;
    context_ = context;
    return true;
}

void FFAVFormat::setStartTime(double start_time, double first_dts) {
    if (start_time_.load() == AV_NOPTS_VALUE) {
        start_time_.store(start_time * AV_TIME_BASE);
        first_dts_.store(first_dts * AV_TIME_BASE);
    }
}

std::shared_ptr<AVStream> FFAVFormat::getStream(int stream_index) const {
    if (stream_index < 0 || stream_index >= (int)context_->nb_streams) {
        return nullptr;
    }

    AVStream *stream = context_->streams[stream_index];
    return std::shared_ptr<AVStream>(stream, [](auto){});
}

std::shared_ptr<AVFormatContext> FFAVFormat::GetContext() const {
    return context_;
}

std::string FFAVFormat::GetURI() const {
    return uri_;
}

uint32_t FFAVFormat::GetStreamNum() const {
    return context_->nb_streams;
}

void FFAVFormat::SetDebug(bool debug) {
    debug_.store(debug);
}

bool FFAVFormat::ReachedEOF() const {
    return reached_eof_.load();
}

void FFAVFormat::DumpStreams() const {
    int is_output = context_->iformat ? 0 : 1;
    av_dump_format(context_.get(), 0, uri_.c_str(), is_output);
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

    auto context_ptr = FFAVFormat::initialize(uri, std::shared_ptr<AVFormatContext>(
        context,
        [&](AVFormatContext* ctx) {
            avformat_close_input(&ctx);
        }
    ));

    for (uint32_t i = 0; i < context->nb_streams; i++) {
        if (!initDecodeStream(i))
            return false;
    }

    return context_ptr;
}

bool FFAVDemuxer::initDecodeStream(int stream_index) {
    if (streams_.count(stream_index))
        return true;

    auto stream = getStream(stream_index);
    if (!stream)
        return false;

    auto decode_stream = FFAVDecodeStream::Create(context_, stream);
    if (!decode_stream)
        return false;

    streams_[stream_index] = decode_stream;
    return true;
}

std::shared_ptr<FFAVStream> FFAVDemuxer::GetDemuxStream(int stream_index) const {
    return GetDecodeStream(stream_index);
}

std::shared_ptr<FFAVDecodeStream> FFAVDemuxer::GetDecodeStream(int stream_index) const {
    return streams_.count(stream_index) ? streams_.at(stream_index) : nullptr;
}

std::string FFAVDemuxer::GetMetadata(const std::string& metakey) const {
    AVDictionaryEntry *entry = av_dict_get(context_->metadata, metakey.c_str(), nullptr, AV_DICT_IGNORE_SUFFIX);
    if (!entry)
        return {};
    return entry->value;
}

std::shared_ptr<AVPacket> FFAVDemuxer::ReadPacket() {
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return nullptr;
    }

    int ret = av_read_frame(context_.get(), packet);
    if (ret < 0) {
        if (ret == AVERROR_EOF)
            reached_eof_.store(true);
        av_packet_free(&packet);
        return nullptr;
    }

    auto stream = getStream(packet->stream_index);
    packet->time_base = stream->time_base;

    if (start_time_.load() == AV_NOPTS_VALUE) {
        int64_t global_pts = av_rescale_q(packet->pts, packet->time_base, AV_TIME_BASE_Q);
        int64_t global_dts = av_rescale_q(packet->dts, packet->time_base, AV_TIME_BASE_Q);
        setStartTime((double)global_pts / AV_TIME_BASE, (double)global_dts / AV_TIME_BASE);
    }

    if (debug_.load())
        std::cout << "[R]" << DumpAVPacket(packet) << std::endl;

    return std::shared_ptr<AVPacket>(packet, [&](AVPacket *p) {
        av_packet_unref(p);
        av_packet_free(&p);
    });
}

bool FFAVDemuxer::Seek(int stream_index, double timestamp) {
    int64_t timestamp_i = timestamp * AV_TIME_BASE;
    auto stream = getStream(stream_index);
    if (stream)
        timestamp_i = av_rescale_q(timestamp_i, AV_TIME_BASE_Q, stream->time_base);
    else
        stream_index = -1;
    int ret = av_seek_frame(context_.get(), stream_index, timestamp_i, AVSEEK_FLAG_BACKWARD);
    if (ret < 0)
        return false;
    return true;
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

    return FFAVFormat::initialize(uri, std::shared_ptr<AVFormatContext>(
        context,
        [&](AVFormatContext* ctx) {
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

std::shared_ptr<FFAVStream> FFAVMuxer::GetMuxStream(int stream_index) const {
    return GetEncodeStream(stream_index);
}

std::shared_ptr<FFAVEncodeStream> FFAVMuxer::GetEncodeStream(int stream_index) const {
    return streams_.count(stream_index) ? streams_.at(stream_index) : nullptr;
}

std::shared_ptr<FFAVStream> FFAVMuxer::AddMuxStream() {
    auto stream = avformat_new_stream(context_.get(), nullptr);
    if (!stream)
        return nullptr;

    auto encode_stream = FFAVEncodeStream::Create(
        context_,
        std::shared_ptr<AVStream>(stream, [](AVStream*){}),
        nullptr);
    if (!encode_stream)
        return nullptr;

    streams_[stream->index] = encode_stream;
    return encode_stream;
}

std::shared_ptr<FFAVEncodeStream> FFAVMuxer::AddEncodeStream(AVCodecID codec_id) {
    if (codec_id == AV_CODEC_ID_NONE)
        return nullptr;

    auto encoder = FFAVEncoder::Create(codec_id);
    if (!encoder)
        return nullptr;

    auto stream = avformat_new_stream(context_.get(), encoder->GetCodec());
    if (!stream)
        return nullptr;

    auto encode_stream = FFAVEncodeStream::Create(
        context_,
        std::shared_ptr<AVStream>(stream, [](AVStream*){}),
        encoder);
    if (!encode_stream)
        return nullptr;

    streams_[stream->index] = encode_stream;
    return encode_stream;
}

bool FFAVMuxer::SetMetadata(const std::unordered_map<std::string, std::string>& metadata) {
    for (const auto& [key, value] : metadata) {
        int ret = av_dict_set(&context_->metadata, key.c_str(), value.c_str(), 0);
        if (ret < 0)
            return false;
    }
    return true;
}

void FFAVMuxer::SetDuration(double duration) {
    if (duration_.load() >= 0)
        return;
    duration_.store(duration * AV_TIME_BASE);
}

bool FFAVMuxer::AllowMux() {
    if (!headmuxed_.load()) {
        if (!writeHeader()) {
            return false;
        }
    }
    return true;
}

bool FFAVMuxer::VerifyMux() {
    if (!trailmuxed_.load()) {
        if (!writeTrailer()) {
            return false;
        }
    }
    return true;
}

bool FFAVMuxer::WritePacket(std::shared_ptr<AVPacket> packet) {
    if (!openMuxer())
        return false;

    auto muxstream = GetMuxStream(packet->stream_index);
    if (!muxstream)
        return false;

    auto muxpacket = muxstream->transformPacket(packet);
    if (!muxpacket)
        return false;

    if (start_time_.load() == AV_NOPTS_VALUE) {
        int64_t global_pts = av_rescale_q(muxpacket->pts, muxpacket->time_base, AV_TIME_BASE_Q);
        int64_t global_dts = av_rescale_q(muxpacket->dts, muxpacket->time_base, AV_TIME_BASE_Q);
        setStartTime((double)global_pts / AV_TIME_BASE, (double)global_dts / AV_TIME_BASE);
    }

    if (duration_.load() > 0) {
        int64_t global_dts = av_rescale_q(muxpacket->dts, muxpacket->time_base, AV_TIME_BASE_Q);
        int64_t duration = global_dts - first_dts_.load() - muxpacket->duration;
        if (duration > duration_.load()) {
            reached_eof_.store(true);
            return false;
        }
    }

    if (strcmp(context_->oformat->name, "flv") == 0) {
        int64_t stream_start_time = av_rescale_q(start_time_.load(), AV_TIME_BASE_Q, muxpacket->time_base);
        int64_t stream_dts_offset = 0;
        if (first_dts_.load() > start_time_.load()) {
            int64_t dts_offset = first_dts_.load() - start_time_.load();
            stream_dts_offset = av_rescale_q(dts_offset, AV_TIME_BASE_Q, muxpacket->time_base);
        }
        muxpacket->pts -= stream_start_time;
        muxpacket->dts -= stream_start_time + stream_dts_offset;
    }

    if (debug_.load())
        std::cout << "[W]" << DumpAVPacket(muxpacket.get()) << std::endl;

    int ret = av_interleaved_write_frame(context_.get(), muxpacket.get());
    if (ret < 0) {
        std::cerr << "av_interleaved_write_frame: " << AVError2Str(ret) << std::endl;
        return false;
    }
    return true;
}
