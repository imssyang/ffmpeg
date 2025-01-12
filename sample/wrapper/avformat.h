#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <deque>
#include <string>
#include "avutil.h"
#include "avcodec.h"
extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
}

class FFAVStream {
public:
    static std::shared_ptr<FFAVStream> Create(
        std::shared_ptr<AVFormatContext> context,
        std::shared_ptr<AVStream> stream);
    std::shared_ptr<AVFormatContext> GetContext() const;
    std::shared_ptr<AVStream> GetStream() const;
    std::shared_ptr<AVCodecParameters> GetParameters() const;
    int GetIndex() const;
    std::string GetMetadata(const std::string& metakey);
    bool SetMetadata(const std::unordered_map<std::string, std::string>& metadata);
    virtual bool SetParameters(const AVCodecParameters& params);
    virtual bool SetTimeBase(const AVRational& time_base);
    virtual ~FFAVStream() = default;

protected:
    FFAVStream() = default;
    bool initialize(
        std::shared_ptr<AVFormatContext> context,
        std::shared_ptr<AVStream> stream);
    AVRational correctTimeBase(const AVRational& time_base);
    std::shared_ptr<AVPacket> transformPacket(std::shared_ptr<AVPacket> packet);

protected:
    std::shared_ptr<AVStream> stream_;
    std::shared_ptr<AVFormatContext> context_;
    friend class FFAVMuxer;
};

class FFAVDecodeStream : public FFAVStream {
public:
    static std::shared_ptr<FFAVDecodeStream> Create(
        std::shared_ptr<AVFormatContext> context,
        std::shared_ptr<AVStream> stream);
    std::shared_ptr<FFAVDecoder> GetDecoder() const;
    std::shared_ptr<AVFrame> ReadFrame(std::shared_ptr<AVPacket> packet = nullptr);
    bool NeedMorePacket() const;
    bool FulledBuffer() const;

private:
    FFAVDecodeStream() = default;
    bool initialize(
        std::shared_ptr<AVFormatContext> context,
        std::shared_ptr<AVStream> stream);
    bool initDecoder();

private:
    std::deque<std::shared_ptr<AVFrame>> frames_;
    std::shared_ptr<FFAVDecoder> decoder_;
};

class FFAVEncodeStream : public FFAVStream {
public:
    static std::shared_ptr<FFAVEncodeStream> Create(
        std::shared_ptr<AVFormatContext> context,
        std::shared_ptr<AVStream> stream,
        std::shared_ptr<FFAVEncoder> encoder);
    std::shared_ptr<FFAVEncoder> GetEncoder() const;
    bool SetParameters(const AVCodecParameters& params) override;
    bool SetTimeBase(const AVRational& time_base) override;
    bool OpenEncoder();
    std::shared_ptr<AVPacket> ReadPacket(std::shared_ptr<AVFrame> frame = nullptr);
    bool NeedMoreFrame() const;
    bool FulledBuffer() const;

private:
    FFAVEncodeStream() = default;
    bool initialize(
        std::shared_ptr<AVFormatContext> context,
        std::shared_ptr<AVStream> stream,
        std::shared_ptr<FFAVEncoder> encoder);
    bool openencoded() const;

private:
    std::atomic_bool openencoded_{false};
    std::deque<std::shared_ptr<AVFrame>> frames_;
    std::shared_ptr<FFAVEncoder> encoder_;
    friend class FFAVMuxer;
};

class FFAVFormat {
protected:
    using AVFormatInitPtr = std::unique_ptr<std::atomic_bool, std::function<void(std::atomic_bool*)>>;

public:
    std::shared_ptr<AVFormatContext> GetContext() const;
    std::string GetURI() const;
    uint32_t GetStreamNum() const;
    void SetDebug(bool debug);
    bool ReachedEOF() const;
    void DumpStreams() const;

protected:
    FFAVFormat() = default;
    bool initialize(const std::string& uri, std::shared_ptr<AVFormatContext> context);
    void setStartTime(double start_time, double first_dts);
    std::shared_ptr<AVStream> getStream(int stream_index) const;

protected:
    static AVFormatInitPtr inited_;
    std::atomic_bool debug_{false};
    std::atomic_bool reached_eof_{false};
    std::atomic_int64_t start_time_{AV_NOPTS_VALUE};
    std::atomic_int64_t first_dts_{AV_NOPTS_VALUE};
    std::string uri_;
    std::shared_ptr<AVFormatContext> context_;
};

class FFAVDemuxer final : public FFAVFormat {
    using FFAVDecodeStreamMap = std::unordered_map<int, std::shared_ptr<FFAVDecodeStream>>;

public:
    static std::shared_ptr<FFAVDemuxer> Create(const std::string& uri);
    std::shared_ptr<FFAVStream> GetDemuxStream(int stream_index) const;
    std::shared_ptr<FFAVDecodeStream> GetDecodeStream(int stream_index) const;
    std::string GetMetadata(const std::string& metakey) const;
    std::shared_ptr<AVPacket> ReadPacket();
    bool Seek(int stream_index, double timestamp);

private:
    FFAVDemuxer() = default;
    bool initialize(const std::string& uri);
    bool initDecodeStream(int stream_index);

private:
    FFAVDecodeStreamMap streams_;
};

class FFAVMuxer final : public FFAVFormat  {
    using FFAVEncodeStreamMap = std::unordered_map<int, std::shared_ptr<FFAVEncodeStream>>;

public:
    static std::shared_ptr<FFAVMuxer> Create(const std::string& uri, const std::string& mux_fmt);
    std::shared_ptr<FFAVStream> GetMuxStream(int stream_index) const;
    std::shared_ptr<FFAVEncodeStream> GetEncodeStream(int stream_index) const;
    std::shared_ptr<FFAVStream> AddMuxStream();
    std::shared_ptr<FFAVEncodeStream> AddEncodeStream(AVCodecID codec_id);
    bool SetMetadata(const std::unordered_map<std::string, std::string>& metadata);
    void SetDuration(double duration);
    bool AllowMux();
    bool VerifyMux();
    bool WritePacket(std::shared_ptr<AVPacket> packet);
    bool WriteFrame(int stream_index, std::shared_ptr<AVFrame> frame);

private:
    FFAVMuxer() = default;
    bool initialize(const std::string& uri, const std::string& mux_fmt);
    bool openMuxer();
    bool writeHeader();
    bool writeTrailer();
    std::shared_ptr<FFAVEncodeStream> openEncoder(int stream_index);

private:
    std::atomic_int64_t duration_{-1};
    std::atomic_bool openmuxed_{false};
    std::atomic_bool headmuxed_{false};
    std::atomic_bool trailmuxed_{false};
    FFAVEncodeStreamMap streams_;
};
