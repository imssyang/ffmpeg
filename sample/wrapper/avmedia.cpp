#include <unordered_set>
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

void FFAVMedia::setDuration(std::shared_ptr<FFAVMuxer> muxer, int stream_index) {
    const auto& uri = muxer->GetURI();
    if (options_.count(uri)) {
        if (options_[uri].count(-1)) {
            auto option = options_[uri][-1];
            if (option.duration > 0) {
                muxer->SetDuration(option.duration);
            }
        }
        if (options_[uri].count(stream_index)) {
            auto option = options_[uri][stream_index];
            if (option.duration > 0) {
                muxer->SetDuration(option.duration);
            }
        }
    }
}

bool FFAVMedia::writePacket(
    std::shared_ptr<FFAVMuxer> muxer,
    std::shared_ptr<AVPacket> packet) {
    if (!muxer->AllowMux())
        return false;

    if (!muxer->WritePacket(packet))
        return false;
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

void FFAVMedia::SetDebug(bool debug) {
    debug_.store(debug);
    for (const auto& item : demuxers_) {
        const auto& demuxer = item.second;
        demuxer->SetDebug(debug);
    }
    for (const auto& item : muxers_) {
        const auto& muxer = item.second;
        muxer->SetDebug(debug);
    }
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

    std::unordered_set<std::string> optseeks;
    std::unordered_set<std::string> optdurations;
    std::unordered_set<std::string> endflags;
    std::unordered_set<std::string> targets;
    while (endflags.size() != rules_.size()) {
        for (const auto& [uri, rules] : rules_) {
            if (endflags.count(uri))
                continue;

            auto demuxer = GetDemuxer(uri);
            if (!demuxer)
                return false;

            if (!optseeks.count(uri)) {
                if (!seekPacket(demuxer))
                    return false;
                optseeks.insert(uri);
            }

            auto packet = demuxer->ReadPacket();
            if (!packet) {
                if (demuxer->ReachedEOF()) {
                    endflags.insert(uri);
                    continue;
                }
                return false;
            }

            if (!rules.count(packet->stream_index))
                continue;

            const auto& target = rules.at(packet->stream_index);
            auto muxer = GetMuxer(target.uri);
            if (!muxer)
                return false;

            if (!optdurations.count(uri)) {
                setDuration(muxer, target.stream_index);
                optdurations.insert(uri);
            }

            packet->stream_index = target.stream_index;
            if (!writePacket(muxer, packet)) {
                if (muxer->ReachedEOF()) {
                    endflags.insert(uri);
                    continue;
                }
                return false;
            }

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

    uint64_t pkgcount = 0, framecount = 0;
    uint64_t enpkgcount = 0;
    std::unordered_set<std::string> optseeks;
    std::unordered_set<std::string> optdurations;
    std::unordered_set<std::string> endpkgs;
    std::unordered_set<std::string> endframes;
    std::unordered_set<std::string> targets;
    while (endframes.size() != rules_.size()) {
        for (const auto& [uri, rules] : rules_) {
            int frame_stream_index = -1;
            std::shared_ptr<AVFrame> frame;
            if (!endframes.count(uri)) {
                auto demuxer = GetDemuxer(uri);
                if (!demuxer)
                    return false;

                if (!endpkgs.count(uri)) {
                    if (!optseeks.count(uri)) {
                        if (!seekPacket(demuxer))
                            return false;
                        optseeks.insert(uri);
                    }

                    auto packet = demuxer->ReadPacket();
                    if (!packet) {
                        if (demuxer->ReachedEOF()) {
                            endpkgs.insert(uri);
                            continue;
                        }
                        return false;
                    }

                    if (!rules.count(packet->stream_index))
                        continue;

                    pkgcount++;
                    //std::cout << "[DR/" << pkgcount
                    //    << "]" << DumpAVPacket(packet.get())
                    //    << std::endl;

                    auto decodestream = demuxer->GetDecodeStream(packet->stream_index);
                    if (!decodestream)
                        return false;

                    frame = decodestream->ReadFrame(packet);
                    if (!frame) {
                        auto decoder = decodestream->GetDecoder();
                        if (decoder->NeedMorePacket())
                            continue;
                        return false;
                    }

                    frame_stream_index = packet->stream_index;
                } else {
                    for (const auto& [stream_index, target] : rules) {
                        auto decodestream = demuxer->GetDecodeStream(stream_index);
                        if (!decodestream)
                            return false;

                        auto decoder = decodestream->GetDecoder();
                        if (decoder->ReachedEOF())
                            continue;

                        frame = decodestream->ReadFrame();
                        if (!frame) {
                            if (decoder->ReachedEOF()) {
                                auto muxer = GetMuxer(target.uri);
                                if (!muxer)
                                    return false;

                                auto encodestream = muxer->GetEncodeStream(target.stream_index);
                                if (!encodestream)
                                    return false;

                                auto encoder = encodestream->GetEncoder();
                                if (!encoder)
                                    return false;

                                encoder->FlushFrame();
                                continue;
                            }
                            return false;
                        }

                        frame_stream_index = stream_index;
                        break;
                    }
                    if (!frame) {
                        endframes.insert(uri);
                        continue;
                    }
                }

                framecount++;
                //std::cout << "[DR/" << framecount
                //    << "]" << DumpAVFrame(frame.get(), frame_stream_index)
                //    << std::endl;
            }

            if (frame_stream_index >= 0) {
                const auto& target = rules.at(frame_stream_index);
                auto muxer = GetMuxer(target.uri);
                if (!muxer)
                    return false;

                if (!optdurations.count(uri)) {
                    setDuration(muxer, target.stream_index);
                    optdurations.insert(uri);
                }

                auto encodestream = muxer->GetEncodeStream(target.stream_index);
                if (!encodestream)
                    return false;

                auto packet = encodestream->ReadPacket(frame);
                if (!packet) {
                    auto encoder = encodestream->GetEncoder();
                    if (encoder->NeedMoreFrame())
                        continue;
                    return false;
                }

                enpkgcount++;
                std::cout << "[ER/" << enpkgcount
                    << "]" << DumpAVPacket(packet.get())
                    << std::endl;

                if (!writePacket(muxer, packet)) {
                    if (muxer->ReachedEOF()) {
                        endpkgs.insert(uri);
                        continue;
                    }
                    return false;
                }

                if (!targets.count(target.uri))
                    targets.insert(target.uri);
            } else {
                //for (const auto& [stream_index, target] : rules) {
                //}
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
