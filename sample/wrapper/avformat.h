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

enum FFAVDirection {
    FFAV_DIRECTION_NONE = -1,
    FFAV_DIRECTION_INPUT,
    FFAV_DIRECTION_OUTPUT
};

class FFAVBaseIO {
protected:
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*)>>;
    using FFAVCodecMap = std::unordered_map<int, std::shared_ptr<FFAVCodec>>;

public:
    std::string GetURI() const;
    FFAVDirection GetDirection() const;
    void DumpStreams() const;
    int GetNumOfStreams() const;
    std::shared_ptr<AVStream> GetStream(int stream_index) const;
    std::shared_ptr<AVStream> GetStreamByID(int stream_id) const;
    std::shared_ptr<AVFrame> Decode(int stream_index, std::shared_ptr<AVPacket> packet);
    std::shared_ptr<AVPacket> Encode(int stream_index, std::shared_ptr<AVFrame> frame);
    bool SetSWScale(
        int stream_index, int dst_width, int dst_height,
        AVPixelFormat dst_pix_fmt, int flags);

protected:
    virtual std::shared_ptr<FFAVCodec> getCodec(int stream_index) = 0;

private:
    int getStreamIndex(int stream_id) const;

protected:
    std::string uri_;
    FFAVDirection direct_;
    FFAVCodecMap codecs_;
    AVFormatContextPtr context_;
    mutable std::recursive_mutex mutex_;
};

class FFAVInput : virtual public FFAVBaseIO {
public:
    static std::shared_ptr<FFAVInput> Create(const std::string& uri);
    virtual ~FFAVInput() = default;
    std::shared_ptr<AVPacket> ReadPacket() const;
    std::shared_ptr<AVFrame> ReadFrame();
    bool Seek(int stream_index, int64_t timestamp);

protected:
    FFAVInput() = default;
    bool initialize(const std::string& uri);
    std::shared_ptr<FFAVCodec> getCodec(int stream_index);
};

class FFAVOutput : virtual public FFAVBaseIO  {
public:
    static std::shared_ptr<FFAVOutput> Create(const std::string& uri, const std::string& mux_fmt);
    virtual ~FFAVOutput() = default;
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
    FFAVOutput() = default;
    bool initialize(const std::string& uri, const std::string& mux_fmt);
    std::shared_ptr<FFAVCodec> getCodec(int stream_index);

private:
    bool open();

private:
    std::atomic_bool opened_;
};

class FFAVFormat : public FFAVInput, public FFAVOutput {
public:
    static std::shared_ptr<FFAVFormat> Create(
        const std::string& uri, const std::string& mux_fmt, FFAVDirection direct);
    static void Destory();

private:
    FFAVFormat() = default;
    bool initialize(const std::string& uri, const std::string& mux_fmt, FFAVDirection direct);
    std::shared_ptr<FFAVCodec> getCodec(int stream_index);

private:
    static std::atomic_bool inited_;
};
