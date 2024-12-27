#pragma once

#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif

class FFmpegMedia {
public:
    FFmpegMedia(const std::string& url);
    ~FFmpegMedia();
    const AVStream* GetStream(int stream_index);
    const AVPacket* GetNextPacket();
    void Show();

private:
    bool Open();

private:
    std::string url_;
    std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>> format_ctx_;
    std::queue<std::unique_ptr<AVPacket, std::function<void(AVPacket*)>>> packet_queues_;
    std::queue<std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>> frame_queues_;
    std::mutex mutex_;
};
