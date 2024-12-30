#include "avformat.h"

int main(int argc, char *argv[]) {
    auto origin = FFAVFormat::Load("/opt/ffmpeg/sample/tiny/oceans.mp4");
    if (origin) {
        origin->DumpStreams();
        for (int i = 0; i < origin->GetNumOfStreams(); i++) {
            std::shared_ptr<AVStream> stream = origin->GetStream(i);
            AVCodecParameters *codecpar = stream->codecpar;
            if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                printf("Found video stream at index %d\n", i);
            } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                printf("Found audio stream at index %d\n", i);
            }
        }
        for (int i = 0; i < 100; i++) {
            std::shared_ptr<AVPacket> packet = origin->ReadPacket();
            std::shared_ptr<AVStream> stream = origin->GetStream(packet->stream_index);
            AVCodecParameters *codecpar = stream->codecpar;
            printf("codec=%d packet: size=%d, pts=%lld\n", codecpar->codec_type, packet->size, packet->pts);
        }
    }
    return 0;
}