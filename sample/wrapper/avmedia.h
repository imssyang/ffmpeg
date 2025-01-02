#pragma once

#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "avformat.h"

struct FFAVNode {
    std::string uri;
    int stream_id;
};

struct FFAVRule {
    FFAVNode src;
    FFAVNode dst;
};

class FFAVMedia {
public:
    static std::shared_ptr<FFAVMedia> Create();
    bool AddDemuxer(const std::string& uri);
    bool AddMuxer(const std::string& uri, const std::string& mux_fmt);
    bool AddRule(const FFAVRule& rule);
    void DumpStreams() const;
    std::shared_ptr<FFAVDemuxer> GetDemuxer(const std::string& uri) const;
    std::shared_ptr<FFAVMuxer> GetMuxer(const std::string& uri) const;
    std::vector<FFAVRule> GetRules(const std::string& uri, bool is_dst);
    std::vector<int> GetStreamIDs(const std::string& uri, AVMediaType media_type);
    std::shared_ptr<AVStream> GetStream(const std::string& uri, int stream_id);
    bool Transcode(int64_t start_timestamp, int duration_ms);

private:
    FFAVMedia() = default;
    bool initialize();

private:
    std::vector<std::shared_ptr<FFAVDemuxer>> demuxers_;
    std::vector<std::shared_ptr<FFAVMuxer>> muxers_;
    std::vector<FFAVRule> rules_;
    mutable std::recursive_mutex mutex_;
};


/*
struct MediaStream {
    int id;
    AVRational time_base;
    int64_t start_time;
    int64_t duration;
    int64_t nb_frames;
};

struct MediaVideo : public MediaStream {
    AVCodecID codec_id;
    AVPixelFormat pix_fmt;
    AVRational framerate;
    int64_t bit_rate;
    int width;
    int height;
    int gop_size;
    int max_b_frames;
    std::string crf;
    std::string preset;
};

struct MediaAudio : public MediaStream {
    AVCodecID codec_id;
    AVSampleFormat sample_fmt;
    int nb_channels;
    uint64_t layout_channel;
    int64_t bit_rate;
    int sample_rate;
};

struct MediaMuxer {
    std::string format;
};

struct MediaParam {
    std::string uri;
    MediaMuxer muxer;
    std::vector<MediaVideo> videos;
    std::vector<MediaAudio> audios;
};

class FFAVMedia {
public:
    static std::shared_ptr<FFAVMedia> Create(
        const std::vector<MediaParam>& origins,
        const std::vector<MediaParam>& targets = {});
    void DumpStreams() const;
    std::vector<int> GetStreamIDs(const std::string& uri, AVMediaType media_type);
    std::shared_ptr<AVStream> GetStream(const std::string& uri, int stream_id);
    std::shared_ptr<FFAVFormat> GetFormat(const std::string& uri);
    bool Transcode();

private:
    FFAVMedia() = default;
    bool initialize(
        const std::vector<MediaParam>& origins,
        const std::vector<MediaParam>& targets);

private:
    std::vector<std::shared_ptr<FFAVFormat>> origins_;
    std::vector<std::shared_ptr<FFAVFormat>> targets_;
    mutable std::recursive_mutex mutex_;
};
*/
