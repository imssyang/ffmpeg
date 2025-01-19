#include <iomanip>
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

std::shared_ptr<AVPacket> FFAVStream::scalePacket(std::shared_ptr<AVPacket> packet) {
    auto scale_pkt = std::shared_ptr<AVPacket>(
        av_packet_clone(packet.get()),
        [&](AVPacket *p) {
            av_packet_unref(p);
            av_packet_free(&p);
        }
    );

    if (context_->iformat) {
        packet_count_++;
        if (packet->time_base.num == 0 || packet->time_base.den == 0) {
            scale_pkt->time_base = stream_->time_base;
        }
    }

    if (context_->oformat) {
        packet_count_++;
        scale_pkt->pts = av_rescale_q_rnd(packet->pts, packet->time_base, stream_->time_base,
            static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        scale_pkt->dts = av_rescale_q_rnd(packet->dts, packet->time_base, stream_->time_base,
            static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        scale_pkt->duration = av_rescale_q(packet->duration, packet->time_base, stream_->time_base);
        scale_pkt->time_base = stream_->time_base;
        scale_pkt->pos = -1;
    }

    if (start_time_.load() == AV_NOPTS_VALUE) {
        start_time_.store(scale_pkt->pts);
        first_dts_.store(scale_pkt->dts);
        pkt_duration_.store(scale_pkt->duration);
        limit_duration_.store(av_rescale_q(limit_duration_.load(), AV_TIME_BASE_Q, stream_->time_base));
    }

    return scale_pkt;
}

std::shared_ptr<AVPacket> FFAVStream::transformPacket(std::shared_ptr<AVPacket> packet) {
    if (context_->oformat) {
        int64_t dts_offset = 0;
        if (first_dts_.load() > start_time_.load())
            dts_offset = first_dts_.load() - start_time_.load();

        packet->pts -= fmt_start_time_.load();
        packet->dts -= fmt_start_time_.load() + dts_offset;
    }
    return packet;
}

bool FFAVStream::checkLimitCondition(std::shared_ptr<AVPacket> packet) {
    if (reached_limit_.load())
        return true;

    if (debug_.load()) {
        std::cout << "[";
        if (context_->iformat)
            std::cout << "R";
        else if (context_->oformat)
            std::cout << "W";
        else
            std::cout << "-";
        std::cout << ":" << packet->stream_index
            << ":" << packet_count_.load()
            << "]" << DumpAVPacket(packet.get())
            << std::endl;
    }

    if (limit_duration_.load() <= 0)
        return false;

    int64_t current_duration = packet->dts - first_dts_.load() - pkt_duration_.load();
    if (current_duration < limit_duration_.load())
        return false;

    reached_limit_.store(true);
    return true;
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

uint64_t FFAVStream::GetPacketCount() const {
    return packet_count_;
}

std::string FFAVStream::GetMetadata(const std::string& metakey) const {
    if (!context_->iformat)
        return {};

    AVDictionaryEntry *entry = av_dict_get(stream_->metadata, metakey.c_str(), nullptr, AV_DICT_IGNORE_SUFFIX);
    if (!entry)
        return {};

    return entry->value;
}

bool FFAVStream::ReachLimit() const {
    return reached_limit_.load();
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

bool FFAVStream::SetDesiredTimeBase(const AVRational& time_base) {
    if (!context_->oformat)
        return false;

    stream_->time_base = time_base;
    return true;
}

void FFAVStream::SetDuration(double duration) {
    if (duration > 0) {
        limit_duration_.store(duration * AV_TIME_BASE);
    }
}

void FFAVStream::SetDebug(bool debug) {
    debug_.store(debug);
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
    if (!initDecoder(stream))
        return false;
    return FFAVStream::initialize(context, stream);
}

bool FFAVDecodeStream::initDecoder(std::shared_ptr<AVStream> stream) {
    if (decoder_)
        return true;

    AVCodecParameters *codecpar = stream->codecpar;
    AVCodecID codec_id = codecpar->codec_id;
    if (codec_id == AV_CODEC_ID_NONE)
        return false;

    auto decoder = FFAVDecoder::Create(codec_id);
    if (!decoder)
        return false;

    if (!decoder->SetParameters(*stream->codecpar))
        return false;

    decoder->SetTimeBase(stream->time_base);
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

    int ret = avcodec_parameters_from_context(stream_->codecpar, encoder_->GetContext());
    if (ret < 0)
        return false;

    stream_->time_base = encoder_->GetContext()->time_base;
    openencoded_.store(true);
    return true;
}

std::shared_ptr<AVFrame> FFAVEncodeStream::transformFrame(std::shared_ptr<AVFrame> frame) {
    auto muxframe = std::shared_ptr<AVFrame>(
        av_frame_clone(frame.get()),
        [&](AVFrame *p) {
            av_frame_unref(p);
            av_frame_free(&p);
        }
    );

    muxframe->pts = av_rescale_q_rnd(frame->pts, frame->time_base, stream_->time_base,
        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    muxframe->pkt_dts = av_rescale_q_rnd(frame->pkt_dts, frame->time_base, stream_->time_base,
        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    muxframe->duration = av_rescale_q(frame->duration, frame->time_base, stream_->time_base);
    muxframe->time_base = stream_->time_base;
    return muxframe;
}

std::shared_ptr<FFAVEncoder> FFAVEncodeStream::GetEncoder() const {
    return encoder_;
}

std::shared_ptr<AVPacket> FFAVEncodeStream::ReadPacket(std::shared_ptr<AVFrame> frame) {
    auto encoder = openEncoder();
    if (!encoder)
        return nullptr;

    auto muxframe = transformFrame(frame);
    if (!muxframe)
        return nullptr;

    auto packet = encoder_->Encode(muxframe);
    if (!packet)
        return nullptr;

    packet->stream_index = stream_->index;
    packet->time_base = stream_->time_base;
    packet->duration = muxframe->duration;
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

std::shared_ptr<FFAVStream> FFAVFormat::getWrapStream(int stream_index) const {
    return streams_.count(stream_index) ? streams_.at(stream_index) : nullptr;
}

std::shared_ptr<AVStream> FFAVFormat::getStream(int stream_index) const {
    if (stream_index < 0 || stream_index >= (int)context_->nb_streams) {
        return nullptr;
    }

    AVStream *stream = context_->streams[stream_index];
    return std::shared_ptr<AVStream>(stream, [](auto){});
}

std::shared_ptr<AVPacket> FFAVFormat::prePacket(std::shared_ptr<AVPacket> packet) {
    auto stream = getWrapStream(packet->stream_index);
    if (!stream) return nullptr;

    auto scale_pkt = stream->scalePacket(packet);
    if (start_time_.load() == AV_NOPTS_VALUE)
        start_time_.store(av_rescale_q(scale_pkt->pts, scale_pkt->time_base, AV_TIME_BASE_Q));

    if (stream->fmt_start_time_.load() == AV_NOPTS_VALUE) {
        auto rawstream = stream->GetStream();
        stream->fmt_start_time_.store(av_rescale_q(start_time_.load(), AV_TIME_BASE_Q, rawstream->time_base));
    }
    return scale_pkt;
}

std::shared_ptr<AVPacket> FFAVFormat::dealPacket(std::shared_ptr<AVPacket> packet) {
    if (!packet) return nullptr;
    auto stream = getWrapStream(packet->stream_index);
    stream->checkLimitCondition(packet);
    return packet;
}

std::shared_ptr<AVPacket> FFAVFormat::postPacket(std::shared_ptr<AVPacket> packet) {
    if (!packet) return nullptr;
    auto stream = getWrapStream(packet->stream_index);
    return stream->transformPacket(packet);
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

std::shared_ptr<FFAVStream> FFAVFormat::GetStream(int stream_index) const {
    return getWrapStream(stream_index);
}

void FFAVFormat::SetDebug(bool debug) {
    debug_.store(debug);
}

void FFAVFormat::SetDuration(double duration) {
    for (auto& item : streams_) {
        auto stream = item.second;
        stream->SetDuration(duration);
    }
}

bool FFAVFormat::ReachEOF() const {
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
        std::cerr << "avformat_open_input(" << uri << "): " << AVErrorStr(ret) << std::endl;
        return false;
    }

    ret = avformat_find_stream_info(context, NULL);
    if (ret < 0) {
        std::cerr << "avformat_find_stream_info(" << uri << "): " << AVErrorStr(ret) << std::endl;
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
        if (!initDemuxStream(i))
            return false;
    }

    return context_ptr;
}

bool FFAVDemuxer::initDemuxStream(int stream_index) {
    if (streams_.count(stream_index))
        return true;

    auto stream = getStream(stream_index);
    if (!stream)
        return false;

    auto demuxstream = FFAVStream::Create(context_, stream);
    if (!demuxstream)
        return false;

    demuxstream->debug_.store(debug_.load());
    streams_[stream_index] = demuxstream;
    return true;
}

std::shared_ptr<FFAVStream> FFAVDemuxer::GetDemuxStream(int stream_index) const {
    return getWrapStream(stream_index);
}

std::shared_ptr<FFAVDecodeStream> FFAVDemuxer::GetDecodeStream(int stream_index) {
    auto demuxstream = getWrapStream(stream_index);
    if (!demuxstream)
        return nullptr;

    auto decodestream = std::dynamic_pointer_cast<FFAVDecodeStream>(demuxstream);
    if (!decodestream) {
        auto decodestream = FFAVDecodeStream::Create(context_, demuxstream->GetStream());
        if (!decodestream)
            return nullptr;

        decodestream->debug_.store(debug_.load());
        streams_[stream_index] = decodestream;
    }
    return decodestream;
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
        if (ret == AVERROR_EOF) {
            reached_eof_.store(true);
            for (auto item : streams_) {
                auto decodestream = GetDecodeStream(item.first);
                if (decodestream) {
                    auto decoder = decodestream->GetDecoder();
                    decoder->FlushPacket();
                }
            }
        }
        av_packet_free(&packet);
        return nullptr;
    }

    auto rawpacket = std::shared_ptr<AVPacket>(packet, [&](AVPacket *p) {
        av_packet_unref(p);
        av_packet_free(&p);
    });

    return dealPacket(prePacket(rawpacket));
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
            << "): " << AVErrorStr(ret) << std::endl;
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
            std::cerr << "avio_open2(" << uri_ << "): " << AVErrorStr(ret) << std::endl;
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

    if (debug_.load()) {
        std::cout << "[W:Header]";
        for (auto& [stream_index, stream] : streams_) {
            auto rawstream = stream->GetStream();
            std::cout << std::fixed << std::setprecision(6)
                << " index:" << stream_index
                << " time_base:" << rawstream->time_base.den;
        }
    }

    int ret = avformat_write_header(context_.get(), nullptr);
    if (ret < 0) {
        std::cerr << "avformat_write_header: " << AVErrorStr(ret) << std::endl;
        return false;
    }

    if (debug_.load()) {
        std::cout << " ->";
        for (auto& [stream_index, stream] : streams_) {
            auto rawstream = stream->GetStream();
            std::cout << std::fixed << std::setprecision(6)
                << " index:" << stream_index
                << " time_base:" << av_q2d(rawstream->time_base)
                << " (" << rawstream->time_base.den << ")";
        }
        std::cout << std::endl;
    }

    headmuxed_.store(true);
    return true;
}

bool FFAVMuxer::writeTrailer() {
    if (trailmuxed_.load())
        return true;

    if (!headmuxed_.load())
        return false;

    if (!openMuxer())
        return false;

    int ret = av_write_trailer(context_.get());
    if (ret < 0) {
        std::cerr << "av_write_trailer: " << AVErrorStr(ret) << std::endl;
        return false;
    }

    trailmuxed_.store(true);
    return true;
}

std::shared_ptr<FFAVStream> FFAVMuxer::GetMuxStream(int stream_index) const {
    return getWrapStream(stream_index);
}

std::shared_ptr<FFAVEncodeStream> FFAVMuxer::GetEncodeStream(int stream_index) const {
    auto muxstream = getWrapStream(stream_index);
    if (!muxstream)
        return nullptr;
    return std::dynamic_pointer_cast<FFAVEncodeStream>(muxstream);
}

std::shared_ptr<FFAVStream> FFAVMuxer::AddMuxStream() {
    auto stream = avformat_new_stream(context_.get(), nullptr);
    if (!stream)
        return nullptr;

    auto muxstream = FFAVStream::Create(
        context_,
        std::shared_ptr<AVStream>(stream, [](auto){}));
    if (!muxstream)
        return nullptr;

    muxstream->debug_.store(debug_.load());
    streams_[stream->index] = muxstream;
    return muxstream;
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

    auto encodestream = FFAVEncodeStream::Create(
        context_,
        std::shared_ptr<AVStream>(stream, [](auto){}),
        encoder);
    if (!encodestream)
        return nullptr;

    encodestream->debug_.store(debug_.load());
    streams_[stream->index] = encodestream;
    return encodestream;
}

bool FFAVMuxer::SetMetadata(const std::unordered_map<std::string, std::string>& metadata) {
    for (const auto& [key, value] : metadata) {
        int ret = av_dict_set(&context_->metadata, key.c_str(), value.c_str(), 0);
        if (ret < 0)
            return false;
    }
    return true;
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

    packet = postPacket(dealPacket(prePacket(packet)));
    if (!packet)
        return false;

    int ret = av_interleaved_write_frame(context_.get(), packet.get());
    if (ret < 0) {
        std::cerr << "av_interleaved_write_frame: " << AVErrorStr(ret) << std::endl;
        return false;
    }
    return true;
}
