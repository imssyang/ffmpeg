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
    FFAVDirection direct;
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
    std::shared_ptr<AVStream> GetVideo(const std::string& uri, int stream_id);
    std::shared_ptr<AVStream> GetAudio(const std::string& uri, int stream_id);
    std::shared_ptr<AVPacket> GetPacket(const std::string& uri);
    std::shared_ptr<AVFrame> GetFrame(const std::string& uri);
    bool Seek(const std::string& uri, AVMediaType media_type, int stream_id, int64_t timestamp);
    bool Transcode();

private:
    FFAVMedia() = default;
    bool initialize(
        const std::vector<MediaParam>& origins,
        const std::vector<MediaParam>& targets);
    std::shared_ptr<FFAVFormat> getFormat(const std::string& uri);

private:
    std::vector<std::shared_ptr<FFAVFormat>> origins_;
    std::vector<std::shared_ptr<FFAVFormat>> targets_;
    mutable std::mutex mutex_;
};
