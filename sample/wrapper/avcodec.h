
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
    bool Opened() const;
    bool SetParameters(const AVCodecParameters *params);
    bool SetSWScale(int dst_width, int dst_height, AVPixelFormat dst_pix_fmt, int flags);
    bool Open();

protected:
    FFAVCodec() = default;
    bool initialize(const AVCodec *codec);

protected:
    AVCodecPtr codec_;
    AVCodecContextPtr context_;
    std::shared_ptr<FFSWScale> swscale_;
    mutable std::recursive_mutex mutex_;

private:
    std::atomic_bool opened_;
};

class FFAVDecoder final : public FFAVCodec {
public:
    static std::shared_ptr<FFAVDecoder> Create(AVCodecID id);
    std::shared_ptr<AVFrame> Decode(std::shared_ptr<AVPacket> packet);

private:
    FFAVDecoder() = default;
    bool initialize(AVCodecID id);
};

class FFAVEncoder final : public FFAVCodec {
public:
    static std::shared_ptr<FFAVEncoder> Create(AVCodecID id);
    void SetFlags(int flags);
    bool SetPrivData(const std::string& name, const std::string& val, int search_flags);
    std::shared_ptr<AVPacket> Encode(std::shared_ptr<AVFrame> frame);

private:
    FFAVEncoder() = default;
    bool initialize(AVCodecID id);
};
