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

class FFAVBaseFormat {
protected:
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>>;

public:
    static void Destory();
    std::string GetURI() const;
    void DumpStreams() const;
    int GetStreamNum() const;
    std::shared_ptr<AVStream> GetStream(int stream_index) const;
    std::shared_ptr<AVStream> GetStreamByID(int stream_id) const;

private:
    int getStreamIndex(int stream_id) const;

protected:
    static std::atomic_bool inited_;
    std::string uri_;
    AVFormatContextPtr context_;
    mutable std::recursive_mutex mutex_;
};

class FFAVDemuxer final : public FFAVBaseFormat {
    using FFAVDecoderMap = std::unordered_map<int, std::shared_ptr<FFAVDecoder>>;

public:
    static std::shared_ptr<FFAVDemuxer> Create(const std::string& uri);
    std::shared_ptr<FFAVDecoder> GetDecoder(int stream_index);
    std::shared_ptr<AVPacket> ReadPacket() const;
    std::shared_ptr<AVFrame> ReadFrame() const;
    bool Seek(int stream_index, int64_t timestamp);

protected:
    FFAVDemuxer() = default;
    bool initialize(const std::string& uri);

private:
    FFAVDecoderMap codecs_;
};

class FFAVMuxer final : public FFAVBaseFormat  {
    using FFAVEncoderMap = std::unordered_map<int, std::shared_ptr<FFAVEncoder>>;

public:
    static std::shared_ptr<FFAVMuxer> Create(const std::string& uri, const std::string& mux_fmt);
    std::shared_ptr<FFAVEncoder> GetEncoder(int stream_index);
    std::shared_ptr<AVStream> AddVideo(
        AVCodecID codec_id,
        AVPixelFormat pix_fmt,
        const AVRational& time_base,
        const AVRational& framerate,
        int64_t bit_rate,
        int width,
        int height,
        int gop_size,
        int max_b_frames,
        int flags,
        const std::string& crf,
        const std::string& preset
    );
    std::shared_ptr<AVStream> AddAudio(
        AVCodecID codec_id,
        AVSampleFormat sample_fmt,
        const AVChannelLayout& ch_layout,
        const AVRational& time_base,
        int64_t bit_rate,
        int sample_rate,
        int flags
    );
    bool WriteHeader();
    bool WriteTrailer();
    bool WritePacket(int stream_index, std::shared_ptr<AVPacket> packet);
    bool WriteFrame(int stream_index, std::shared_ptr<AVFrame> frame);

protected:
    FFAVMuxer() = default;
    bool initialize(const std::string& uri, const std::string& mux_fmt);

private:
    bool open();

private:
    std::atomic_bool opened_;
};

class FFAVFormat {
public:
    static std::shared_ptr<FFAVFormat> Create(
        const std::string& uri, const std::string& mux_fmt, bool isoutput);

private:
    FFAVFormat() = default;
    bool initialize(const std::string& uri, const std::string& mux_fmt, bool isoutput);
    std::shared_ptr<FFAVCodec> getCodec(int stream_index);

private:
    std::shared_ptr<FFAVMuxer> muxer_;
    std::shared_ptr<FFAVDemuxer> demuxer_;
};
