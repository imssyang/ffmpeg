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

struct BaseMedia {
    int id;
    AVRational time_base;
    int64_t start_time;
    int64_t duration;
    int64_t nb_frames;
};

struct MediaVideo : public BaseMedia {
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

struct MediaAudio : public BaseMedia {
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
    std::vector<int> GetIDs(const std::string& uri, AVMediaType media_type);
    std::shared_ptr<MediaVideo> GetVideo(int id);
    std::shared_ptr<MediaAudio> GetAudio(int id);
    std::shared_ptr<AVPacket> GetPacket(AVMediaType media_type, int id);
    std::shared_ptr<AVFrame> GetFrame(AVMediaType media_type, int id);
    bool Seek(const std::string& uri, AVMediaType media_type, int id, int64_t timestamp);
    bool Transcode();

private:
    FFAVMedia() = default;
    bool initialize(
        const std::vector<MediaParam>& origins,
        const std::vector<MediaParam>& targets);

private:
    std::vector<std::shared_ptr<FFAVFormat>> origins_;
    std::vector<std::shared_ptr<FFAVFormat>> targets_;
    mutable std::mutex mutex_;
};
