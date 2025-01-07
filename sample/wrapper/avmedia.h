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

struct FFAVNode {
    std::string uri;
    int stream_index;
};

struct FFAVOption : public FFAVNode {
    int64_t seek_timestamp;
    int duration;
};

class FFAVMedia {
    using FFAVDemuxerMap = std::unordered_map<std::string, std::shared_ptr<FFAVDemuxer>>;
    using FFAVMuxerMap = std::unordered_map<std::string, std::shared_ptr<FFAVMuxer>>;
    using FFAVRuleMap = std::unordered_map<std::string, std::unordered_map<int, FFAVNode>>;
    using FFAVOptionMap = std::unordered_map<std::string, std::unordered_map<int, FFAVOption>>;

public:
    static std::shared_ptr<FFAVMedia> Create();
    std::shared_ptr<FFAVDemuxer> AddDemuxer(const std::string& uri);
    std::shared_ptr<FFAVMuxer> AddMuxer(const std::string& uri, const std::string& mux_fmt);
    bool AddRule(const FFAVNode& src, const FFAVNode& dst);
    bool SetOption(const FFAVOption& opt);
    bool Transcode();

    std::shared_ptr<FFAVDemuxer> GetDemuxer(const std::string& uri) const;
    std::shared_ptr<FFAVMuxer> GetMuxer(const std::string& uri) const;
    std::shared_ptr<AVStream> GetStream(const std::string& uri, int stream_index) const;
    void DumpStreams() const;

private:
    FFAVMedia() = default;
    bool initialize();
    bool writeMuxer(const std::string& uri, int stream_index, std::shared_ptr<AVFrame> frame);

private:
    mutable std::recursive_mutex mutex_;
    FFAVDemuxerMap demuxers_;
    FFAVMuxerMap muxers_;
    FFAVRuleMap rules_;
    FFAVOptionMap options_;
};
