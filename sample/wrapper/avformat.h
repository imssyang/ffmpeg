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
    bool NeedMorePacket() const;
    bool FulledBuffer() const;

private:
    FFAVDecodeStream() = default;
    bool initialize(std::shared_ptr<AVStream> stream, bool enable_decode);

private:
    std::shared_ptr<FFAVDecoder> decoder_;
};

class FFAVEncodeStream : public FFAVStream {
public:
    static std::shared_ptr<FFAVEncodeStream> Create(std::shared_ptr<AVStream> stream, std::shared_ptr<FFAVEncoder> encoder);
    std::shared_ptr<FFAVEncoder> GetEncoder() const;
    bool SetParameters(const AVCodecParameters& params);
    bool SetTimeBase(const AVRational& time_base);
    bool OpenEncoder();
    bool Openencoded() const;
    std::shared_ptr<AVPacket> ReadPacket(std::shared_ptr<AVFrame> frame = nullptr);
    bool NeedMoreFrame() const;
    bool FulledBuffer() const;

private:
    FFAVEncodeStream() = default;
    bool initialize(std::shared_ptr<AVStream> stream, std::shared_ptr<FFAVEncoder> encoder);

private:
    std::shared_ptr<FFAVEncoder> encoder_;
    std::atomic_bool openencoded_{false};
};

class FFAVFormat {
protected:
    using AVFormatInitPtr = std::unique_ptr<std::atomic_bool, std::function<void(std::atomic_bool*)>>;
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>>;

public:
    std::string GetURI() const;
    uint32_t GetStreamNum() const;
    std::shared_ptr<AVStream> GetStream(int stream_index) const;

protected:
    FFAVFormat() = default;
    bool initialize(const std::string& uri, AVFormatContextPtr context);
    void dumpStreams(int is_output) const;

protected:
    mutable std::recursive_mutex mutex_;
    static AVFormatInitPtr inited_;
    std::string uri_;
    AVFormatContextPtr context_;
};

class FFAVDemuxer final : public FFAVFormat {
    using FFAVDecodeStreamMap = std::unordered_map<int, std::shared_ptr<FFAVDecodeStream>>;

public:
    static std::shared_ptr<FFAVDemuxer> Create(const std::string& uri);
    std::shared_ptr<FFAVDecodeStream> GetDecodeStream(int stream_index) const;
    std::shared_ptr<AVPacket> ReadPacket() const;
    bool Seek(int stream_index, int64_t timestamp);
    void DumpStreams() const;

private:
    FFAVDemuxer() = default;
    bool initialize(const std::string& uri);
    std::shared_ptr<FFAVDecodeStream> initStream(int stream_index);

private:
    FFAVDecodeStreamMap streams_;
};

class FFAVMuxer final : public FFAVFormat  {
    using FFAVEncodeStreamMap = std::unordered_map<int, std::shared_ptr<FFAVEncodeStream>>;

public:
    static std::shared_ptr<FFAVMuxer> Create(const std::string& uri, const std::string& mux_fmt);
    std::shared_ptr<FFAVEncodeStream> GetEncodeStream(int stream_index) const;
    std::shared_ptr<FFAVEncodeStream> AddStream(AVCodecID codec_id = AV_CODEC_ID_NONE);
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
    std::shared_ptr<FFAVEncodeStream> openEncoder(int stream_index);

private:
    std::atomic_bool openmuxed_{false};
    std::atomic_bool headmuxed_{false};
    std::atomic_bool trailmuxed_{false};
    FFAVEncodeStreamMap streams_;
};
