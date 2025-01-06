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

std::shared_ptr<FFAVDemuxer> FFAVMedia::GetDemuxer(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return demuxers_.count(uri) ? demuxers_.at(uri) : nullptr;
}

std::shared_ptr<FFAVMuxer> FFAVMedia::GetMuxer(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return muxers_.count(uri) ? muxers_.at(uri) : nullptr;
}

std::vector<FFAVRule> FFAVMedia::GetRules(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<FFAVRule> rules;
    for (auto& rule : rules_) {
        if (rule.src.uri == uri || rule.dst.uri == uri) {
            rules.emplace_back(rule);
        }
    }
    return rules;
}

std::vector<FFAVRule> FFAVMedia::GetRules(const std::string& uri, bool is_dst) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<FFAVRule> rules;
    for (auto& rule : rules_) {
        auto c_uri = is_dst ? rule.dst.uri : rule.src.uri;
        if (c_uri == uri) {
            rules.emplace_back(rule);
        }
    }
    return rules;
}

std::vector<FFAVRule> FFAVMedia::GetRules(const std::string& uri, int stream_index, bool is_dst) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<FFAVRule> rules;
    for (auto& rule : rules_) {
        auto c_uri = is_dst ? rule.dst.uri : rule.src.uri;
        auto c_stream_index = is_dst ? rule.dst.stream_index : rule.src.stream_index;
        if (c_uri == uri && c_stream_index == stream_index) {
            rules.emplace_back(rule);
        }
    }
    return rules;
}

std::vector<int> FFAVMedia::GetStreamIDs(const std::string& uri, AVMediaType media_type) {
    std::vector<int> streamIDs;
    auto demuxer = GetDemuxer(uri);
    if (demuxer) {
        int nb_streams = demuxer->GetStreamNum();
        for (int i = 0; i < nb_streams; i++) {
            auto stream = demuxer->GetStream(i);
            if (media_type == stream->codecpar->codec_type) {
                streamIDs.push_back(stream->id);
            }
        }
    } else {
        auto muxer = GetMuxer(uri);
        if (muxer) {
            int nb_streams = muxer->GetStreamNum();
            for (int i = 0; i < nb_streams; i++) {
                auto stream = demuxer->GetStream(i);
                if (media_type == stream->codecpar->codec_type) {
                    streamIDs.push_back(stream->id);
                }
            }
        }
    }
    return streamIDs;
}

std::shared_ptr<AVStream> FFAVMedia::GetStream(const std::string& uri, int stream_index) {
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

bool FFAVMedia::AddRule(const FFAVRule& rule) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    rules_.emplace_back(rule);
    return true;
}

bool FFAVMedia::Transcode() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (demuxers_.empty() || muxers_.empty() || rules_.empty())
        return false;

    for (auto& rule : rules_) {
        auto muxer = GetMuxer(rule.dst.uri);
        if (muxer) {
            muxer->WriteHeader();
        }
    }
    for (int i = 0; i < 3000; i++) {
        //std::cout << "rules: " << rules_.size() << std::endl;
        for (auto& rule : rules_) {
            auto demuxer = GetDemuxer(rule.src.uri);
            if (!demuxer) {
                return false;
            }

            if (!rule.src.seeked) {
                auto stream = demuxer->GetStream(rule.src.stream_index);
                if (!stream) {
                    return false;
                }

                bool seek_ok = demuxer->Seek(stream->index, rule.src.seek_timestamp);
                if (seek_ok) {
                    rule.src.seeked = true;
                }
                continue;
            }

            auto packet = demuxer->ReadPacket();
            if (packet->stream_index != rule.src.stream_index) {
                //std::cerr << "Ignore frame" << rule.src.uri << std::endl;
                continue;
            }

            auto frame = demuxer->Decode(packet);
            if (!frame) {
                std::cerr << "No frame" << rule.src.uri << std::endl;
                continue;
            }

            std::cout << "stream:" << packet->stream_index 
                    << " key_frame:" << frame->key_frame
                    << " dts:" << frame->pkt_dts
                    << " pts:" << frame->pts
                    << std::endl;
            auto muxer = GetMuxer(rule.dst.uri);
            if (muxer) {
                auto stream = muxer->GetStream(rule.dst.stream_index);
                if (!stream) {
                    return false;
                }

                bool write_ok = muxer->WriteFrame(stream->index, frame);
                if (!write_ok) {
                    //return false;
                }
            }
        }
    }
    for (auto& rule : rules_) {
        auto muxer = GetMuxer(rule.dst.uri);
        if (muxer) {
            muxer->WriteTrailer();
        }
    }
    return true;
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
