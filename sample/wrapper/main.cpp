#include <stdio.h>
extern "C" {
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return -1;
    }

    const char *input_filename = argv[1];

    // 初始化 FFmpeg
    avformat_network_init();

    // 打开输入文件
    AVFormatContext *format_ctx = NULL;
    if (avformat_open_input(&format_ctx, input_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open input file '%s'\n", input_filename);
        return -1;
    }

    // 获取流信息
    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    // 打印媒体文件信息
    av_dump_format(format_ctx, 0, input_filename, 0);

    // 查找音频和视频流
    int video_stream_index = -1;
    int audio_stream_index = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        AVStream *stream = format_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            printf("Found video stream at index %d\n", i);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            printf("Found audio stream at index %d\n", i);
        }
    }

    // 读取数据包
    AVPacket pkt;
    while (av_read_frame(format_ctx, &pkt) >= 0) {
        if (pkt.stream_index == video_stream_index) {
            printf("Video packet: size=%d, pts=%lld\n", pkt.size, pkt.pts);
        } else if (pkt.stream_index == audio_stream_index) {
            printf("Audio packet: size=%d, pts=%lld\n", pkt.size, pkt.pts);
        }
        // 释放数据包
        av_packet_unref(&pkt);
    }

    // 释放资源
    avformat_close_input(&format_ctx);
    avformat_network_deinit();

    return 0;
}
