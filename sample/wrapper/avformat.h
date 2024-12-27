#pragma once

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

class AVFormat {
public:
    AVFormat(const std::string& url);
    ~AVFormat();
    void DumpStreams() const;
    int NumOfStreams() const;
    std::shared_ptr<AVStream> GetStream(int index) const;
    std::shared_ptr<AVPacket> ReadNextPacket() const;
    bool SaveFormat(const std::string& url);

private:
    bool LoadStreams();

private:
    std::string url_;
    std::unique_ptr<
        AVFormatContext,
        std::function<void(AVFormatContext*)>
    > context_;
    mutable std::mutex mutex_;
};
