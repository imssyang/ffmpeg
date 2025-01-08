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

bool FFAVMedia::writeMuxer(const std::string& uri, int stream_index, std::shared_ptr<AVFrame> frame, bool last_frame) {
    auto muxer = GetMuxer(uri);
    if (!muxer)
        return false;

    if (!muxer->AllowWrite()) {
        return false;
    }

    if (!muxer->WriteFrame(stream_index, frame)) {
        return false;
    }

    if (last_frame) {
        if (!muxer->VerifyMuxer()) {
            return false;
        }
    }

    return true;
}

bool FFAVMedia::Remux() {
    return true;
}

bool FFAVMedia::Transcode() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (demuxers_.empty() || muxers_.empty() || rules_.empty())
        return false;

    for (int i = 0; i < 500; i++) {
        for (const auto& [uri, rules] : rules_) {
            auto demuxer = GetDemuxer(uri);
            if (!demuxer) {
                return false;
            }

            auto packet = demuxer->ReadPacket();
            if (packet) {
                if (!rules.count(packet->stream_index))
                    continue;
                //PrintAVPacket(packet.get());

                auto frame = demuxer->Decode(packet->stream_index, packet);
                if (!frame)
                    continue;

                //PrintAVFrame(frame.get(), packet->stream_index);
                const auto& target = rules.at(packet->stream_index);
                if (!writeMuxer(target.uri, target.stream_index, frame, i == 499))
                    return false;
            } else {
                for (const auto& [stream_index, target] : rules) {
                    auto frame = demuxer->Decode(stream_index);
                    if (!frame)
                        continue;

                    //PrintAVFrame(frame.get(), stream_index);
                    if (!writeMuxer(target.uri, target.stream_index, frame, i == 499))
                        return false;
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

void FFAVMedia::DumpStreams(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto demuxer = GetDemuxer(uri);
    if (demuxer)
        demuxer->DumpStreams();

    auto muxer = GetMuxer(uri);
    if (muxer)
        muxer->DumpStreams();
}
