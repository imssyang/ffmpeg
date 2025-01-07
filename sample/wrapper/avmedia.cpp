#include "avmedia.h"

std::shared_ptr<FFAVMedia> FFAVMedia::Create() {
    auto instance = std::shared_ptr<FFAVMedia>(new FFAVMedia());
    if (!instance->initialize())
        return nullptr;
    return instance;
}

bool FFAVMedia::initialize() {
    return true;
}

std::shared_ptr<FFAVDemuxer> FFAVMedia::AddDemuxer(const std::string& uri) {
    auto demuxer = FFAVDemuxer::Create(uri);
    if (!demuxer)
        return nullptr;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    demuxers_[uri] = demuxer;
    return demuxer;
}

std::shared_ptr<FFAVMuxer> FFAVMedia::AddMuxer(const std::string& uri, const std::string& mux_fmt) {
    auto muxer = FFAVMuxer::Create(uri, mux_fmt);
    if (!muxer)
        return nullptr;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    muxers_[uri] = muxer;
    return muxer;
}

bool FFAVMedia::AddRule(const FFAVNode& src, const FFAVNode& dst) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!rules_.count(src.uri))
        rules_[src.uri] = {};
    rules_[src.uri][src.stream_index] = dst;
    return true;
}

bool FFAVMedia::SetOption(const FFAVOption& opt) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!options_.count(opt.uri))
        options_[opt.uri] = {};
    options_[opt.uri][opt.stream_index] = opt;
    return true;
}

bool FFAVMedia::writeMuxer(const std::string& uri, int stream_index, std::shared_ptr<AVFrame> frame) {
    auto muxer = GetMuxer(uri);
    if (!muxer)
        return false;

    auto stream = muxer->GetStream(stream_index);
    if (!stream)
        return false;

    if (!muxer->WriteHeader())
        return false;

    int ret = muxer->WriteFrame(stream->index, frame);
    if (ret < 0) {
        //if (ret_write == AVERROR(EAGAIN))
        //AVERROR(EAGAIN) input is not accepted in the current state - user must read output with avcodec_receive_packet() (once all output is read, the packet should be resent, and the call will not fail with EAGAIN).
        //AVERROR_EOF the encoder has been flushed, and no new frames can be sent to it
        //AVERROR(EINVAL) codec not opened, it is a decoder, or requires flush
        //AVERROR(ENOMEM) failed to add packet to internal queue, or similar
        //"another negative error code" legitimate encoding errors
    }

    if (!muxer->WriteTrailer())
        return false;
    return true;
}

bool FFAVMedia::Transcode() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (demuxers_.empty() || muxers_.empty() || rules_.empty())
        return false;

    for (int i = 0; i < 1000; i++) {
        for (const auto& [uri, rules] : rules_) {
            auto demuxer = GetDemuxer(uri);
            if (!demuxer) {
                return false;
            }

            auto packet = demuxer->ReadPacket();
            if (packet) {
                if (!rules.count(packet->stream_index))
                    continue;
                PrintAVPacket(packet.get());

                auto frame = demuxer->Decode(packet->stream_index, packet);
                if (!frame)
                    continue;
                PrintAVFrame(frame.get(), packet->stream_index);

                const auto& target = rules.at(packet->stream_index);
            } else {
                for (const auto& [stream_index, target] : rules) {
                    auto frame = demuxer->Decode(stream_index);
                    if (!frame)
                        continue;
                    PrintAVFrame(frame.get(), stream_index);
                }
            }
        }
    }
    return true;
}

std::shared_ptr<FFAVDemuxer> FFAVMedia::GetDemuxer(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return demuxers_.count(uri) ? demuxers_.at(uri) : nullptr;
}

std::shared_ptr<FFAVMuxer> FFAVMedia::GetMuxer(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return muxers_.count(uri) ? muxers_.at(uri) : nullptr;
}

std::shared_ptr<AVStream> FFAVMedia::GetStream(const std::string& uri, int stream_index) const {
    auto demuxer = GetDemuxer(uri);
    if (demuxer) {
        return demuxer->GetStream(stream_index);
    }

    auto muxer = GetMuxer(uri);
    if (muxer) {
        return muxer->GetStream(stream_index);
    }

    return nullptr;
}

void FFAVMedia::DumpStreams() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto item : demuxers_) {
        auto demuxer = item.second;
        demuxer->DumpStreams();
    }
    for (auto item : muxers_) {
        auto muxer = item.second;
        muxer->DumpStreams();
    }
}
