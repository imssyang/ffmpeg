#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include "swscale.h"
extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

class FFAVCodec {
    struct NoOpDeleter { template <typename T> void operator()(T*) const {} };
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext*)>>;
    using AVCodecPtr = std::unique_ptr<const AVCodec, NoOpDeleter>;

public:
    static std::shared_ptr<FFAVCodec> Create(AVCodecID id);
    AVCodecContext* GetContext() const;
    const AVCodec* GetCodec() const;
    std::shared_ptr<FFSWScale> GetSWScale() const;
    std::shared_ptr<AVCodecParameters> GetParameters() const;
    bool SetParameters(const AVCodecParameters *params);
    void SetCodecType(AVMediaType codec_type);
    void SetBitRate(int64_t bit_rate);
    void SetWidth(int width);
    void SetHeight(int height);
    void SetTimeBase(const AVRational& time_base);
    void SetFrameRate(const AVRational& framerate);
    void SetGopSize(int gop_size);
    void SetMaxBFrames(int max_b_frames);
    void SetPixFmt(AVPixelFormat pix_fmt);
    void SetSampleRate(int sample_rate);
    void SetSampleFormat(AVSampleFormat sample_fmt);
    void SetChannelLayout(const AVChannelLayout& ch_layout);
    void SetFlags(int flags);
    bool SetPrivData(const std::string& name, const std::string& val, int search_flags);
    bool Open();
    bool SetSWScale(int dst_width, int dst_height, AVPixelFormat dst_pix_fmt, int flags);
    std::shared_ptr<AVFrame> Decode(std::shared_ptr<AVPacket> packet);
    std::shared_ptr<AVPacket> Encode(std::shared_ptr<AVFrame> frame);

private:
    FFAVCodec(AVCodecID id);
    bool initialize();

private:
    AVCodecID id_;
    AVCodecPtr codec_;
    AVCodecContextPtr context_;
    std::atomic_bool opened_;
    std::shared_ptr<FFSWScale> swscale_;
    mutable std::mutex mutex_;
};
