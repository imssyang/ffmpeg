#include "avformat.h"

int main(int argc, char *argv[]) {
    AVFormat av("/opt/ffmpeg/sample/tiny/oceans.mp4");
    av.DumpStreams();
    for (int i = 0; i < av.NumOfStreams(); i++) {
        std::shared_ptr<AVStream> stream = av.GetStream(i);
        AVCodecParameters *codecpar = stream->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("Found video stream at index %d\n", i);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("Found audio stream at index %d\n", i);
        }
    }
    for (int i = 0; i < 100; i++) {
        std::shared_ptr<AVPacket> packet = av.ReadNextPacket();
        std::shared_ptr<AVStream> stream = av.GetStream(packet->stream_index);
        AVCodecParameters *codecpar = stream->codecpar;
        printf("codec=%d packet: size=%d, pts=%lld\n", codecpar->codec_type, packet->size, packet->pts);
    }
    av.SaveFormat("oceans.mkv");
    return 0;
}