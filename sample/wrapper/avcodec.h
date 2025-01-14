
#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include "avutil.h"
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
    AVCodecContext* GetContext() const;
    const AVCodec* GetCodec() const;
    std::shared_ptr<AVCodecParameters> GetParameters() const;
    std::shared_ptr<FFSWScale> GetSWScale() const;
    void SetDebug(bool debug);
    bool SetSWScale(int dst_width, int dst_height, AVPixelFormat dst_pix_fmt, int flags);
    bool Open();
    bool ReachedEOF() const;
    bool FulledBuffer() const;

protected:
    FFAVCodec() = default;
    bool initialize(const AVCodec *codec);

protected:
    mutable std::recursive_mutex mutex_;
    std::atomic_bool debug_{false};
    std::atomic_bool reached_eof_{false};
    std::atomic_bool fulled_buffer_{false};
    AVCodecPtr codec_;
    AVCodecContextPtr context_;
    std::shared_ptr<FFSWScale> swscale_;

private:
    std::atomic_bool opened_;
};

class FFAVDecoder final : public FFAVCodec {
public:
    static std::shared_ptr<FFAVDecoder> Create(AVCodecID id);
    bool SetParameters(const AVCodecParameters& params);
    void SetTimeBase(const AVRational& time_base);
    std::shared_ptr<AVFrame> Decode(std::shared_ptr<AVPacket> packet);
    bool NeedMorePacket() const;
    bool FlushPacket();

private:
    FFAVDecoder() = default;
    bool initialize(AVCodecID id);
    bool sendPacket(std::shared_ptr<AVPacket> packet);
    std::shared_ptr<AVFrame> recvFrame();

private:
    std::atomic_bool fulled_buffer_{false};
    std::atomic_bool need_more_packet_{true};
    std::atomic_bool no_any_packet_{false};
};

class FFAVEncoder final : public FFAVCodec {
public:
    static std::shared_ptr<FFAVEncoder> Create(AVCodecID id);
    bool SetParameters(const AVCodecParameters& params);
    void SetTimeBase(const AVRational& time_base);
    void SetSampleAspectRatio(const AVRational& sar);
    void SetGopSize(int gop_size);
    void SetMaxBFrames(int max_b_frames);
    void SetFlags(int flags);
    bool SetPrivData(const std::string& name, const std::string& val, int search_flags);
    std::shared_ptr<AVPacket> Encode(std::shared_ptr<AVFrame> frame);
    bool NeedMoreFrame() const;
    bool FlushFrame();

private:
    FFAVEncoder() = default;
    bool initialize(AVCodecID id);
    bool sendFrame(std::shared_ptr<AVFrame> frame);
    std::shared_ptr<AVPacket> recvPacket();

private:
    std::atomic_bool need_more_frame_{true};
    std::atomic_bool no_any_frame_{false};
};

std::string DumpAVPacket(const AVPacket* packet);
std::string DumpAVCodecParameters(const AVCodecParameters* params);
