#pragma once

#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "avutil.h"
#include "avformat.h"

struct FFAVSrc {
    std::string uri;
    int stream_index;
    int64_t seek_timestamp;
    int duration;
    bool seeked;
};

struct FFAVDst {
    std::string uri;
    int stream_index;
};

struct FFAVRule {
    FFAVSrc src;
    FFAVDst dst;
};

class FFAVMedia {
    using FFAVDemuxerMap = std::unordered_map<std::string, std::shared_ptr<FFAVDemuxer>>;
    using FFAVMuxerMap = std::unordered_map<std::string, std::shared_ptr<FFAVMuxer>>;

public:
    static std::shared_ptr<FFAVMedia> Create();
    std::shared_ptr<FFAVDemuxer> GetDemuxer(const std::string& uri) const;
    std::shared_ptr<FFAVMuxer> GetMuxer(const std::string& uri) const;
    std::vector<FFAVRule> GetRules(const std::string& uri) const;
    std::vector<FFAVRule> GetRules(const std::string& uri, bool is_dst) const;
    std::vector<FFAVRule> GetRules(const std::string& uri, int stream_index, bool is_dst) const;
    std::vector<int> GetStreamIDs(const std::string& uri, AVMediaType media_type);
    std::shared_ptr<AVStream> GetStream(const std::string& uri, int stream_index);
    std::shared_ptr<FFAVDemuxer> AddDemuxer(const std::string& uri);
    std::shared_ptr<FFAVMuxer> AddMuxer(const std::string& uri, const std::string& mux_fmt);
    bool AddRule(const FFAVRule& rule);
    bool Transcode();
    void DumpStreams() const;

private:
    FFAVMedia() = default;
    bool initialize();

private:
    FFAVDemuxerMap demuxers_;
    FFAVMuxerMap muxers_;
    std::vector<FFAVRule> rules_;
    mutable std::recursive_mutex mutex_;
};
