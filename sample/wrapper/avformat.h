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
protected:
    using AVFormatInitPtr = std::unique_ptr<std::atomic_bool, std::function<void(std::atomic_bool*)>>;
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>>;

public:
    std::string GetURI() const;
    uint32_t GetStreamNum() const;
    std::shared_ptr<AVStream> GetStream(int stream_index) const;
    std::shared_ptr<AVStream> GetStreamByID(int stream_id) const;

protected:
    FFAVFormat() = default;
    bool initialize(const std::string& uri, AVFormatContextPtr context);
    int toStreamIndex(int stream_id) const;
    void dumpStreams(int is_output) const;

protected:
    static AVFormatInitPtr inited_;
    std::string uri_;
    AVFormatContextPtr context_;
    mutable std::recursive_mutex mutex_;
};

class FFAVDemuxer final : public FFAVFormat {
    using FFAVDecoderMap = std::unordered_map<int, std::shared_ptr<FFAVDecoder>>;

public:
    static std::shared_ptr<FFAVDemuxer> Create(const std::string& uri);
    std::shared_ptr<FFAVDecoder> GetDecoder(int stream_index);
    std::shared_ptr<AVPacket> ReadPacket() const;
    std::shared_ptr<AVFrame> ReadFrame();
    bool Seek(int stream_index, int64_t timestamp);
    void DumpStreams() const;

private:
    FFAVDemuxer() = default;
    bool initialize(const std::string& uri);
    std::shared_ptr<FFAVDecoder> openDecoder(int stream_index);

private:
    FFAVDecoderMap codecs_;
};

class FFAVMuxer final : public FFAVFormat  {
    using FFAVEncoderMap = std::unordered_map<int, std::shared_ptr<FFAVEncoder>>;

public:
    static std::shared_ptr<FFAVMuxer> Create(const std::string& uri, const std::string& mux_fmt);
    std::shared_ptr<FFAVEncoder> GetEncoder(int stream_index);
    std::shared_ptr<AVStream> AddStream(bool enable_encode, AVCodecID codec_id, const AVRational* time_base);
    bool SetParams(int stream_index, AVCodecParameters *params);
    bool WriteHeader();
    bool WriteTrailer();
    bool WritePacket(int stream_index, std::shared_ptr<AVPacket> packet);
    bool WriteFrame(int stream_index, std::shared_ptr<AVFrame> frame);
    void DumpStreams() const;

private:
    FFAVMuxer() = default;
    bool initialize(const std::string& uri, const std::string& mux_fmt);
    bool openMuxer();
    std::shared_ptr<FFAVEncoder> openEncoder(int stream_index);

private:
    std::atomic_bool opened_;
    FFAVEncoderMap codecs_;
};
