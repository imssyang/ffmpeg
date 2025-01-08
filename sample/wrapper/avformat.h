#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "avutil.h"
#include "avcodec.h"
extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
}

class FFAVStream {
public:
    std::shared_ptr<AVStream> GetStream() const;

protected:
    FFAVStream() = default;
    bool initialize(std::shared_ptr<AVStream> stream);

protected:
    std::shared_ptr<AVStream> stream_;
};

class FFAVDecodeStream : public FFAVStream {
public:
    static std::shared_ptr<FFAVDecodeStream> Create(std::shared_ptr<AVStream> stream, bool enable_decode);
    std::shared_ptr<AVFrame> ReadFrame(std::shared_ptr<AVPacket> packet = nullptr);
    bool Available() const;
    bool Flushed() const;

private:
    FFAVDecodeStream() = default;
    bool initialize(std::shared_ptr<AVStream> stream, bool enable_decode);

private:
    std::shared_ptr<FFAVDecoder> decoder_;
};

class FFAVEncodeStream : public FFAVStream {
public:
    static std::shared_ptr<FFAVEncodeStream> Create(std::shared_ptr<AVStream> stream, bool enable_encode);
    bool SetParameters(const AVCodecParameters& params);
    std::shared_ptr<FFAVEncoder> GetEncoder() const;

private:
    FFAVEncodeStream() = default;
    bool initialize(std::shared_ptr<AVStream> stream, bool enable_encode);

private:
    std::shared_ptr<FFAVEncoder> encoder_;
};

class FFAVFormat {
protected:
    using AVFormatInitPtr = std::unique_ptr<std::atomic_bool, std::function<void(std::atomic_bool*)>>;
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>>;

public:
    std::string GetURI() const;
    uint32_t GetStreamNum() const;

protected:
    FFAVFormat() = default;
    bool initialize(const std::string& uri, AVFormatContextPtr context);
    std::shared_ptr<AVStream> getStream(int stream_index) const;
    void dumpStreams(int is_output) const;

protected:
    mutable std::recursive_mutex mutex_;
    static AVFormatInitPtr inited_;
    std::string uri_;
    AVFormatContextPtr context_;
};

class FFAVDemuxer final : public FFAVFormat {
    using FFAVStreamMap = std::unordered_map<int, std::shared_ptr<FFAVDecodeStream>>;

public:
    static std::shared_ptr<FFAVDemuxer> Create(const std::string& uri);
    std::shared_ptr<FFAVDecodeStream> GetStream(int stream_index) const;
    std::shared_ptr<AVPacket> ReadPacket() const;
    bool Seek(int stream_index, int64_t timestamp);
    void DumpStreams() const;

private:
    FFAVDemuxer() = default;
    bool initialize(const std::string& uri);
    std::shared_ptr<FFAVDecodeStream> initStream(int stream_index);

private:
    FFAVStreamMap streams_;
};

class FFAVMuxer final : public FFAVFormat  {
    using FFAVEncoderMap = std::unordered_map<int, std::shared_ptr<FFAVEncoder>>;

public:
    static std::shared_ptr<FFAVMuxer> Create(const std::string& uri, const std::string& mux_fmt);
    std::shared_ptr<FFAVEncoder> GetEncoder(int stream_index);
    std::shared_ptr<AVStream> AddStream(AVCodecID codec_id = AV_CODEC_ID_NONE);
    bool SetTimeBase(int stream_index, const AVRational& time_base);
    bool SetParams(int stream_index, const AVCodecParameters& params);
    bool AllowWrite();
    bool VerifyMuxer();
    bool WritePacket(std::shared_ptr<AVPacket> packet);
    bool WriteFrame(int stream_index, std::shared_ptr<AVFrame> frame);
    void DumpStreams() const;

private:
    FFAVMuxer() = default;
    bool initialize(const std::string& uri, const std::string& mux_fmt);
    bool openMuxer();
    bool writeHeader();
    bool writeTrailer();
    void setEncoderFlags(int stream_index);
    std::shared_ptr<FFAVEncoder> openEncoder(int stream_index);

private:
    std::atomic_bool opened_;
    std::atomic_bool headed_;
    std::atomic_bool trailed_;
    FFAVEncoderMap codecs_;
};
