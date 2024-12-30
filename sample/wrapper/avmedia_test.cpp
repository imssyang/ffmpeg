#include "avmedia.h"

int main22() {
    auto v0 = MediaVideo{
        0, { 1, 30 }, 0, 0, 0,
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
        0, { 1, 48000 }, 0, 0, 0,
        AV_CODEC_ID_AAC,
        AV_SAMPLE_FMT_U8,
        1,
        AV_CH_LAYOUT_MONO,
        1024,
        48000
    };
    auto i0 = MediaParam{
        "/opt/ffmpeg/sample/tiny/oceans.mp4"
    };
    auto i1 = MediaParam{ "", {}, { v0 }, { a0 } };
    auto o0 = MediaParam{
        "/opt/ffmpeg/sample/tiny/oceans.mp4", { "mp4" }
    };
    auto o1 = MediaParam{
        "/opt/ffmpeg/sample/tiny/oceans-o.mp4", { "mp4" },
        { v0 }, { a0 }
    };
    auto m0 = FFAVMedia::Create({ i0 });
    auto m1 = FFAVMedia::Create({ i0 }, { o0 });
    auto m2 = FFAVMedia::Create({ i0 }, { o1 });
    auto m3 = FFAVMedia::Create({ i1 }, { o0 });
    auto m4 = FFAVMedia::Create({ i1 }, { o1 });
    return 0;
}