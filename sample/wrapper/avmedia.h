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

struct VideoCodec {
    AVCodecID codec_id;
    AVPixelFormat pix_fmt;
    AVRational time_base;
    AVRational framerate;
    int64_t bit_rate;
    int width;
    int height;
    int gop_size;
    int max_b_frames;
    int flags;
    std::string crf;
    std::string preset;
};

struct AudioCodec {
    AVCodecID codec_id;
    AVSampleFormat sample_fmt;
    AVChannelLayout ch_layout;
    AVRational time_base;
    int64_t bit_rate;
    int sample_rate;
    int flags;
};

struct MuxerParam {
    std::string format;
};

struct MediaOrigin {
    std::string url;
};

struct MediaTarget {
    std::string url;
    MuxerParam muxer;
    std::vector<VideoCodec> video_codecs;
    std::vector<AudioCodec> audio_codecs;
};

struct MediaParam {
    std::vector<MediaOrigin> origins;
    std::vector<MediaTarget> targets;
};

class FFAVMedia {
    using FFAVFormatMap = std::unordered_map<std::string, std::shared_ptr<FFAVFormat>>;

public:
    static std::shared_ptr<FFAVMedia> Create(const MediaParam& param);
    std::vector<int> GetVideoStreamIDs(const std::string& url);
    std::vector<int> GetAudioStreamIDs(const std::string& url);
    std::shared_ptr<VideoCodec> GetVideoCodec(const std::string& url, int stream_id);
    std::shared_ptr<AudioCodec> GetAudioCodec(const std::string& url, int stream_id);

private:
    FFAVMedia() = default;
    bool initialize(const MediaParam& param);

private:
    FFAVFormatMap inputs_;
    FFAVFormatMap outputs_;
    mutable std::mutex mutex_;
};
