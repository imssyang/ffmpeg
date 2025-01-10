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
    std::shared_ptr<AVPacket> TransformPacket(std::shared_ptr<AVPacket> packet);
    virtual bool SetParameters(const AVCodecParameters& params);
    virtual bool SetTimeBase(const AVRational& time_base);
    virtual ~FFAVStream() = default;

protected:
    FFAVStream() = default;
    bool initialize(
        std::shared_ptr<AVFormatContext> context,
        std::shared_ptr<AVStream> stream);
    void savePacket(std::shared_ptr<AVPacket> packet);
    AVRational correctTimeBase(const AVRational& time_base);

protected:
    const uint32_t packet_max{3};
    std::deque<std::shared_ptr<AVPacket>> packets_;
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
    bool Openencoded() const;
    std::shared_ptr<AVPacket> ReadPacket(std::shared_ptr<AVFrame> frame = nullptr);
    bool NeedMoreFrame() const;
    bool FulledBuffer() const;

private:
    FFAVEncodeStream() = default;
    bool initialize(
        std::shared_ptr<AVFormatContext> context,
        std::shared_ptr<AVStream> stream,
        std::shared_ptr<FFAVEncoder> encoder);

private:
    std::atomic_bool openencoded_{false};
    std::deque<std::shared_ptr<AVFrame>> frames_;
    std::shared_ptr<FFAVEncoder> encoder_;
};

class FFAVFormat {
protected:
    using AVFormatInitPtr = std::unique_ptr<std::atomic_bool, std::function<void(std::atomic_bool*)>>;

public:
    std::shared_ptr<AVFormatContext> GetContext() const;
    std::string GetURI() const;
    uint32_t GetStreamNum() const;
    std::shared_ptr<AVPacket> GetFirstPacket() const;
    void DumpStreams() const;

protected:
    FFAVFormat() = default;
    bool initialize(const std::string& uri, std::shared_ptr<AVFormatContext> context);
    void savePacket(std::shared_ptr<AVPacket> packet);
    std::shared_ptr<AVStream> getStream(int stream_index) const;

protected:
    static AVFormatInitPtr inited_;
    std::shared_ptr<AVPacket> first_packet_;
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
    bool Seek(int stream_index, int64_t timestamp);
    bool ReachedEOF() const;

private:
    FFAVDemuxer() = default;
    bool initialize(const std::string& uri);
    bool initDecodeStream(int stream_index);

private:
    std::atomic_bool reached_eof_{false};
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
    std::atomic_bool openmuxed_{false};
    std::atomic_bool headmuxed_{false};
    std::atomic_bool trailmuxed_{false};
    FFAVEncodeStreamMap streams_;
};
