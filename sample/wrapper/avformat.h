#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "avcodec.h"
extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
}

class FFAVFormat {
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>>;
    using FFAVCodecMap = std::unordered_map<int, std::shared_ptr<FFAVCodec>>;

public:
    static std::shared_ptr<FFAVFormat> Load(const std::string& url);
    static std::shared_ptr<FFAVFormat> Create(const std::string& url, const std::string& format_name);
    static void Cleanup();
    void DumpStreams() const;
    int GetNumOfStreams() const;
    std::shared_ptr<AVStream> GetStream(int stream_index) const;
    std::shared_ptr<AVPacket> ReadPacket() const;
    std::shared_ptr<AVFrame> ReadFrame();
    std::shared_ptr<AVStream> AddVideoStream(
        AVCodecID codec_id,
        AVPixelFormat pix_fmt,
        const AVRational& time_base,
        const AVRational& framerate,
        int64_t bit_rate,
        int width,
        int height,
        int gop_size,
        int max_b_frames,
        int flags,
        const std::string& crf,
        const std::string& preset
    );
    std::shared_ptr<AVStream> AddAudioStream(
        AVCodecID codec_id,
        AVSampleFormat sample_fmt,
        const AVChannelLayout& ch_layout,
        const AVRational& time_base,
        int64_t bit_rate,
        int sample_rate,
        int flags
    );
    bool WriteHeader();
    bool WriteTrailer();
    bool WritePacket(int stream_index, std::shared_ptr<AVPacket> packet);
    bool WriteFrame(int stream_index, std::shared_ptr<AVFrame> frame);
    bool SetSWScale(
        int stream_index, int dst_width, int dst_height,
        AVPixelFormat dst_pix_fmt, int flags);

private:
    FFAVFormat(const std::string& url, bool isout);
    bool initialize(const std::string& format_name);
    std::shared_ptr<FFAVCodec> getInputCodec(int stream_index);
    std::shared_ptr<FFAVCodec> getOutputCodec(int stream_index);

private:
    static std::atomic_bool isinit_;
    std::atomic_bool isout_;
    std::atomic_bool opened_;
    std::string url_;
    FFAVCodecMap codecs_;
    AVFormatContextPtr context_;
    mutable std::mutex mutex_;
};
