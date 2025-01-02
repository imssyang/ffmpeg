#include "avmedia.h"

void test_avcodec() {

}

void test_avformat() {
    auto origin = FFAVFormat::Create("/opt/ffmpeg/sample/tiny/oceans.mp4", "mp4", FFAV_DIRECTION_INPUT);
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
}

void test_avmedia() {
    auto v0 = MediaVideo{
        { 0, { 1, 30 }, 0, 0, 0 },
        AV_CODEC_ID_H264,
        AV_PIX_FMT_YUV420P,
        { 30, 1 },
        1024000,
        1920,
        1080,
        0,
        0,
        "30",
        "slow"
    };
    auto a0 = MediaAudio{
        { 0, { 1, 48000 }, 0, 0, 0 },
        AV_CODEC_ID_AAC,
        AV_SAMPLE_FMT_U8,
        1,
        AV_CH_LAYOUT_MONO,
        1024,
        48000
    };
    auto i0 = MediaParam{
        FFAV_DIRECTION_INPUT,
        "/opt/ffmpeg/sample/tiny/oceans.mp4",
        {}, {}, {}
    };
    auto i1 = MediaParam{
        FFAV_DIRECTION_INPUT, "", {}, { v0 }, { a0 }
    };
    auto o0 = MediaParam{
        FFAV_DIRECTION_OUTPUT,
        "/opt/ffmpeg/sample/tiny/oceans.mp4", { "mp4" }, {}, {}
    };
    auto o1 = MediaParam{
        FFAV_DIRECTION_OUTPUT,
        "/opt/ffmpeg/sample/tiny/oceans-o.mp4", { "mp4" },
        { v0 }, { a0 }
    };
    auto m0 = FFAVMedia::Create({ i0 });
    if (m0) {
        m0->DumpStreams();
        auto videoIDs = m0->GetStreamIDs(i0.uri, AVMEDIA_TYPE_VIDEO);
        for (auto id : videoIDs) {
            auto video = m0->GetStream(i0.uri, id);
            std::cout << "video[" << video->id << "]: " << video->duration << std::endl;
        }
        auto audioIDs = m0->GetStreamIDs(i0.uri, AVMEDIA_TYPE_AUDIO);
        for (auto id : audioIDs) {
            auto audio = m0->GetStream(i0.uri, id);
            std::cout << "audio[" << audio->id << "]: " << audio->duration << std::endl;
        }
        auto format = m0->GetFormat(i0.uri);
        if (format) {
            //auto packet = format->ReadPacket();
            //std::cout << packet->size << std::endl;
            auto frame = format->ReadFrame();
            std::cout << frame->width << std::endl;
        }
    }

    auto m1 = FFAVMedia::Create({ i0 }, { o0 });
    auto m2 = FFAVMedia::Create({ i0 }, { o1 });
    auto m3 = FFAVMedia::Create({ i1 }, { o0 });
    auto m4 = FFAVMedia::Create({ i1 }, { o1 });
}

int main() {
    test_avcodec();
    //test_avformat();
    //test_avmedia();
    return 0;
}
