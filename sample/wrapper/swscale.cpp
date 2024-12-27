#include "swscale.h"

FFSwsCale::FFSwsCale(
    int srcW, int srcH, enum AVPixelFormat srcPixFmt,
    int dstW, int dstH, enum AVPixelFormat dstPixFmt,
    int flags
) : srcWidth_(srcW), srcHeight_(srcH), srcPixFmt_(srcPixFmt)
  , dstWidth_(dstW), dstHeight_(dstH), dstPixFmt_(dstPixFmt)
  , flags_(flags) {
}

bool FFSwsCale::SetSrcFilter(
    float lumaGBlur, float chromaGBlur, float lumaSharpen,
    float chromaSharpen, float chromaHShift, float chromaVShift,
    int verbose
) {
    SwsFilter* filter = sws_getDefaultFilter(
        lumaGBlur, chromaGBlur, lumaSharpen,
        chromaSharpen,  chromaHShift,  chromaVShift, verbose);
    if (!filter)
        return false;

    srcFilter_ = SwsFilterPtr(filter, [](SwsFilter *fp) {
        sws_freeFilter(fp);
    });
    return true;
}

bool FFSwsCale::SetDstFilter(
    float lumaGBlur, float chromaGBlur, float lumaSharpen,
    float chromaSharpen, float chromaHShift, float chromaVShift,
    int verbose
) {
    SwsFilter* filter = sws_getDefaultFilter(
        lumaGBlur, chromaGBlur, lumaSharpen,
        chromaSharpen,  chromaHShift,  chromaVShift, verbose);
    if (!filter)
        return false;

    dstFilter_ = SwsFilterPtr(filter, [](SwsFilter *fp) {
        sws_freeFilter(fp);
    });
    return true;
}

bool FFSwsCale::SetParams(const std::vector<double> params) {
    return true;
}

