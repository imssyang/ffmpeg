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


class FFSwsCale {
    using SwsContextPtr = std::unique_ptr<SwsContext, std::function<void(SwsContext*)>>;
    using SwsFilterPtr = std::unique_ptr<SwsFilter, std::function<void(SwsFilter*)>>;

public:
    FFSwsCale(
        int srcW, int srcH, enum AVPixelFormat srcPixFmt,
        int dstW, int dstH, enum AVPixelFormat dstPixFmt,
        int flags);
    bool SetSrcFilter(
        float lumaGBlur, float chromaGBlur, float lumaSharpen,
        float chromaSharpen, float chromaHShift, float chromaVShift,
        int verbose);
    bool SetDstFilter(
        float lumaGBlur, float chromaGBlur, float lumaSharpen,
        float chromaSharpen, float chromaHShift, float chromaVShift,
        int verbose);
    bool SetParams(const std::vector<double> params);

private:
    int srcWidth_;
    int srcHeight_;
    int dstWidth_;
    int dstHeight_;
    enum AVPixelFormat srcPixFmt_;
    enum AVPixelFormat dstPixFmt_;
    SwsFilterPtr srcFilter_;
    SwsFilterPtr dstFilter_;
    int flags_;
    std::vector<double> params_;
    SwsContextPtr context_;
    mutable std::mutex mutex_;
};
