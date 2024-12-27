#include "media.h"

FFmpegMedia::FFmpegMedia(const std::string& url)
    : url_(url) {
    avformat_network_init();
}

FFmpegMedia::~FFmpegMedia() {
    avformat_network_deinit();
}

bool FFmpegMedia::Open() {
    AVFormatContext *format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, url_.c_str(), NULL, NULL) < 0) {
        fprintf(stderr, "Could not open input file '%s'\n", url_);
        return false;
    }

    format_ctx_ = std::make_unique<AVFormatContext>(format_ctx, [](AVFormatContext* ctx) {
        avformat_close_input(&ctx);
    });

    if (avformat_find_stream_info(format_ctx_.get(), NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return false;
    }

    return true;
}

const AVStream* FFmpegMedia::GetStream(int stream_index) {
    return format_ctx_->streams[stream_index];
}

const AVPacket* FFmpegMedia::GetNextPacket() {
    AVPacket pkt;
    while (av_read_frame(format_ctx_, &pkt) >= 0) {
        const AVStream *stream = GetStream(pkt.stream_index);
        const AVCodecParameters *codecpar = stream->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("Video packet: size=%d, pts=%lld\n", pkt.size, pkt.pts);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("Audio packet: size=%d, pts=%lld\n", pkt.size, pkt.pts);
        }
        av_packet_unref(&pkt);
    }
    return nullptr;
}

void FFmpegMedia::Show() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!format_ctx_)
        return;

    for (unsigned int i = 0; i < format_ctx_->nb_streams; i++) {
        av_dump_format(format_ctx_, i, url_.c_str(), 0);
    }
}

