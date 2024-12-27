#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
extern "C" {
#define __STDC_CONSTANT_MACROS
#include "libavformat/avformat.h"
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}


class FFAVCodec {
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext*)>>;

public:
    static std::shared_ptr<FFAVCodec> Create(enum AVCodecID id);
    void SetBitRate(int64_t bit_rate);
    void SetWidth(int width);
    void SetHeight(int height);
    void SetTimeBase(const AVRational& time_base);
    void SetFrameRate(const AVRational& framerate);
    void SetGopSize(int gop_size);
    void SetMaxBFrames(int max_b_frames);
    void SetPixFmt(enum AVPixelFormat pix_fmt);
    void SetSampleRate(int sample_rate);
    void SetSampleFormat(enum AVSampleFormat sample_fmt);
    void SetChannelLayout(uint64_t channel_layout);
    void SetFlags(int flags);
    bool SetPrivData(const std::string& name, const std::string& val, int search_flags = 0);
    bool Open(AVStream *stream);

private:
    FFAVCodec(enum AVCodecID id);
    bool initialize();

private:
    enum AVCodecID id_;
    AVCodecContextPtr context_;
    mutable std::mutex mutex_;
};
