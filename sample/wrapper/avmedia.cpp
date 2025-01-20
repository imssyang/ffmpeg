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

bool FFAVMedia::seekPacket(std::shared_ptr<FFAVDemuxer> demuxer) {
    const auto& uri = demuxer->GetURI();
    for (const auto& [stream_index, option] : options_[uri]) {
        if (option.seek_timestamp > 0) {
            if (!demuxer->Seek(stream_index, option.seek_timestamp)) {
                return false;
            }
        }
    }
    return true;
}

bool FFAVMedia::setDuration(std::shared_ptr<FFAVFormat> avformat) {
    const auto& uri = avformat->GetURI();
    for (const auto& [stream_index, option] : options_[uri]) {
        if (option.duration > 0) {
            if (stream_index < 0) {
                avformat->SetDuration(option.duration);
            } else {
                auto stream = avformat->GetStream(stream_index);
                if (!stream)
                    return false;
                stream->SetDuration(option.duration);
            }
        }
    }
    return true;
}

std::shared_ptr<AVPacket> FFAVMedia::readPacket(std::shared_ptr<FFAVDemuxer> demuxer) {
    const auto& uri = demuxer->GetURI();
    if (!optseeks_.count(uri)) {
        if (!seekPacket(demuxer))
            return nullptr;

        optseeks_.insert(uri);
    }

    if (!optdurations_.count(uri)) {
        if (!setDuration(demuxer))
            return nullptr;
        optdurations_.insert(uri);
    }

    return demuxer->ReadPacket();
}

std::pair<int, std::shared_ptr<AVFrame>> FFAVMedia::readFrame(std::shared_ptr<FFAVDemuxer> demuxer) {
    const auto& uri = demuxer->GetURI();
    int frame_stream_index = -1;
    std::shared_ptr<AVFrame> frame;
    while (true) {
        if (!endpackets_.count(uri)) {
            auto packet = readPacket(demuxer);
            if (!packet) {
                if (demuxer->ReachEOF()) {
                    endpackets_.insert(uri);
                    continue;
                }
                return { -1, nullptr };
            }

            if (rules_.count(uri)) {
                if (!rules_[uri].count(packet->stream_index))
                    continue;
            }

            auto decodestream = demuxer->GetDecodeStream(packet->stream_index);
            if (!decodestream)
                return { -1, nullptr };

            if (decodestream->ReachLimit())
                continue;

            frame = decodestream->ReadFrame(packet);
            if (!frame) {
                auto decoder = decodestream->GetDecoder();
                if (decoder->NeedMorePacket())
                    continue;
                return { -1, nullptr };
            }
            frame_stream_index = packet->stream_index;
        } else {
            for (uint32_t stream_index = 0; stream_index < demuxer->GetStreamNum(); stream_index++) {
                auto decodestream = demuxer->GetDecodeStream(stream_index);
                if (!decodestream)
                    return { -1, nullptr };

                auto decoder = decodestream->GetDecoder();
                if (decoder->ReachEOF())
                    continue;

                frame = decodestream->ReadFrame();
                if (!frame) {
                    if (decoder->ReachEOF()) {
                        if (rules_.count(uri) && rules_[uri].count(stream_index)) {
                            const auto& target = rules_[uri][stream_index];
                            auto muxer = GetMuxer(target.uri);
                            if (!muxer)
                                return { -1, nullptr };

                            auto encodestream = muxer->GetEncodeStream(target.stream_index);
                            if (!encodestream)
                                return { -1, nullptr };

                            auto encoder = encodestream->GetEncoder();
                            if (!encoder)
                                return { -1, nullptr };

                            encoder->FlushFrame();
                        }
                        continue;
                    }
                    return { -1, nullptr };
                }
                frame_stream_index = stream_index;
                break;
            }
        }
        break;
    }

    return {frame_stream_index, frame};
}

bool FFAVMedia::writePacket(std::shared_ptr<FFAVMuxer> muxer, std::shared_ptr<AVPacket> packet) {
    const auto& uri = muxer->GetURI();
    if (!optdurations_.count(uri)) {
        if (!setDuration(muxer))
            return false;
        optdurations_.insert(uri);
    }

    if (!muxer->AllowMux())
        return false;

    if (!muxer->WritePacket(packet))
        return false;
    return true;
}

bool FFAVMedia::writeFrame(std::shared_ptr<FFAVMuxer> muxer, int stream_index, std::shared_ptr<AVFrame> frame) {
    auto encodestream = muxer->GetEncodeStream(stream_index);
    if (!encodestream)
        return false;

    auto encoder = encodestream->GetEncoder();
    if (encoder->ReachEOF())
        return false;

    auto packet = encodestream->ReadPacket(frame);
    if (!packet)
        return false;

    return writePacket(muxer, packet);
}

std::shared_ptr<FFAVDemuxer> FFAVMedia::GetDemuxer(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return demuxers_.count(uri) ? demuxers_.at(uri) : nullptr;
}

std::shared_ptr<FFAVMuxer> FFAVMedia::GetMuxer(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return muxers_.count(uri) ? muxers_.at(uri) : nullptr;
}

void FFAVMedia::SetDebug(bool debug) {
    debug_.store(debug);
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

std::shared_ptr<FFAVDemuxer> FFAVMedia::AddDemuxer(const std::string& uri) {
    auto demuxer = FFAVDemuxer::Create(uri);
    if (!demuxer)
        return nullptr;

    demuxer->SetDebug(debug_.load());
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    demuxers_[uri] = demuxer;
    return demuxer;
}

std::shared_ptr<FFAVMuxer> FFAVMedia::AddMuxer(const std::string& uri, const std::string& mux_fmt) {
    auto muxer = FFAVMuxer::Create(uri, mux_fmt);
    if (!muxer)
        return nullptr;

    muxer->SetDebug(debug_.load());
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
    options_[opt.uri][opt.stream_index] = opt;
    return true;
}

bool FFAVMedia::Remux() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (demuxers_.empty() || muxers_.empty() || rules_.empty())
        return false;

    std::unordered_set<std::string> targets;
    while (endpackets_.size() != rules_.size()) {
        for (const auto& [uri, rules] : rules_) {
            if (endpackets_.count(uri))
                continue;

            auto demuxer = GetDemuxer(uri);
            if (!demuxer)
                return false;

            auto packet = readPacket(demuxer);
            if (!packet) {
                if (demuxer->ReachEOF()) {
                    endpackets_.insert(uri);
                    continue;
                }
                return false;
            }

            auto demuxstream = demuxer->GetDemuxStream(packet->stream_index);
            if (demuxstream->ReachLimit())
                continue;

            if (!rules.count(packet->stream_index))
                continue;

            const auto& target = rules.at(packet->stream_index);
            auto muxer = GetMuxer(target.uri);
            if (!muxer)
                return false;

            packet->stream_index = target.stream_index;
            auto muxstream = muxer->GetMuxStream(packet->stream_index);
            if (muxstream->ReachLimit())
                continue;

            if (!writePacket(muxer, packet))
                return false;

            if (!targets.count(target.uri))
                targets.insert(target.uri);
        }
    }

    for (const auto& uri : targets) {
        auto muxer = GetMuxer(uri);
        if (!muxer)
            return false;

        if (!muxer->VerifyMux())
            return false;
    }

    return true;
}

bool FFAVMedia::Transcode() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (demuxers_.empty() || muxers_.empty() || rules_.empty())
        return false;

    std::unordered_set<std::string> endflags;
    std::unordered_set<std::string> endframes;
    std::unordered_set<std::string> targets;
    while (endflags.size() != rules_.size()) {
        for (const auto& [uri, rules] : rules_) {
            if (!endframes.count(uri)) {
                auto demuxer = GetDemuxer(uri);
                if (!demuxer)
                    return false;

                auto [stream_index, frame] = readFrame(demuxer);
                if (!frame) {
                    endframes.insert(uri);
                    continue;
                }

                const auto& target = rules.at(stream_index);
                auto muxer = GetMuxer(target.uri);
                if (!muxer)
                    return false;

                auto encodestream = muxer->GetEncodeStream(target.stream_index);
                if (!encodestream)
                    return false;

                if (encodestream->ReachLimit())
                    continue;

                if (!writeFrame(muxer, target.stream_index, frame)) {
                    auto encoder = encodestream->GetEncoder();
                    if (encoder->NeedMoreFrame())
                        continue;
                    return false;
                }

                if (!targets.count(target.uri))
                    targets.insert(target.uri);
            } else {
                bool all_eof = true;
                for (const auto& [stream_index, target] : rules) {
                    auto muxer = GetMuxer(target.uri);
                    if (!muxer)
                        return false;

                    auto encodestream = muxer->GetEncodeStream(target.stream_index);
                    if (!encodestream)
                        return false;

                    if (encodestream->ReachLimit())
                        continue;

                    if (!writeFrame(muxer, target.stream_index, nullptr)) {
                        auto encoder = encodestream->GetEncoder();
                        if (encoder->ReachEOF())
                            continue;
                        return false;
                    }

                    all_eof = false;
                    if (!targets.count(target.uri))
                        targets.insert(target.uri);
                }
                if (all_eof) {
                    endflags.insert(uri);
                    continue;
                }
            }
        }
    }

    for (const auto& uri : targets) {
        auto muxer = GetMuxer(uri);
        if (!muxer)
            return false;

        if (!muxer->VerifyMux())
            return false;
    }

    return true;
}
