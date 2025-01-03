#include "avmedia.h"

void test_avcodec() {

}

void test_avformat() {
    auto origin = FFAVDemuxer::Create("/opt/ffmpeg/sample/tiny/oceans.mp4");
    if (origin) {
        origin->DumpStreams();
        for (uint32_t i = 0; i < origin->GetStreamNum(); i++) {
            std::shared_ptr<AVStream> stream = origin->GetStream(i);
            AVCodecParameters *codecpar = stream->codecpar;
            if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                printf("Found video stream at index %d\n", i);
            } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                printf("Found audio stream at index %d\n", i);
            }
        }
        for (int i = 0; i < 20; i++) {
            std::shared_ptr<AVPacket> packet = origin->ReadPacket();
            std::shared_ptr<AVStream> stream = origin->GetStream(packet->stream_index);
            AVCodecParameters *codecpar = stream->codecpar;
            std::cout << codecpar->codec_type << " size:" << packet->size << " pts:" << packet->pts << std::endl;
        }
    }
}

void test_avmedia() {
    auto src_uri = "/opt/ffmpeg/sample/tiny/oceans.mp4";
    auto src_video_id = 100, src_audio_id = 101;
    auto src_video = FFAVSrc{ src_uri, src_video_id, 0, 10, true };
    auto src_audio = FFAVSrc{ src_uri, src_audio_id, 0, 10, true };
    auto dst_uri = "/opt/ffmpeg/sample/tiny/oceans-o.flv";
    auto dst_video_id = 200, dst_audio_id = 201;
    auto dst_video = FFAVDst{ dst_uri, dst_video_id };
    auto dst_audio = FFAVDst{ dst_uri, dst_audio_id };
    AVCodecParameters dst_video_params{};
    dst_video_params.codec_type = AVMEDIA_TYPE_VIDEO;
    dst_video_params.codec_id = AV_CODEC_ID_H264;
    dst_video_params.bit_rate = 1024000;
    dst_video_params.format = AV_PIX_FMT_YUV420P;
    dst_video_params.width = 640;
    dst_video_params.height = 480;
    dst_video_params.color_range = AVCOL_RANGE_MPEG;
    dst_video_params.color_primaries = AVCOL_PRI_BT709;
    dst_video_params.color_trc = AVCOL_TRC_BT709;
    dst_video_params.color_space = AVCOL_SPC_BT709;
    dst_video_params.chroma_location = AVCHROMA_LOC_LEFT;
    dst_video_params.sample_aspect_ratio = { 16, 9 };
    dst_video_params.video_delay = 1;
    dst_video_params.framerate = { 1, 30 };
    AVCodecParameters dst_audio_params{};
    dst_audio_params.codec_type = AVMEDIA_TYPE_AUDIO;
    dst_audio_params.codec_id = AV_CODEC_ID_AAC;
    dst_audio_params.bit_rate = 102400;
    dst_audio_params.format = AV_SAMPLE_FMT_U8;
    dst_audio_params.ch_layout = AV_CHANNEL_LAYOUT_MONO;
    dst_audio_params.sample_rate = 48000;
    auto m = FFAVMedia::Create();
    auto demuxer = m->AddDemuxer(src_uri);
    for (auto i = 0; i < demuxer->GetStreamNum(); i++) {
        auto src_stream = demuxer->GetStream(i);
        if (src_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            src_stream->id = src_video_id;
        } else if (src_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            src_stream->id = src_audio_id;
        }
    }
    auto muxer = m->AddMuxer(dst_uri, "flv");
    auto video = muxer->AddStream(true, dst_video_params.codec_id, dst_video_params.framerate);
    auto audio = muxer->AddStream(true, dst_audio_params.codec_id, { 1, 48000 });
    muxer->SetStreamID(video->index, dst_stream_id);
    muxer->SetParams(video->index, dst_video_params);
    muxer->SetParams(audio->index, dst_audio_params);
    m->AddRule({ src, dst });
    m->DumpStreams();
    m->Transcode();
}

int main() {
    try {
        //test_avcodec();
        //test_avformat();
        test_avmedia();
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred." << std::endl;
        return 1;
    }
    return 0;
}
