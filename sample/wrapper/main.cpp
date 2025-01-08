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


void test_remux() {
    auto m = FFAVMedia::Create();

    auto src_uri = "/opt/ffmpeg/sample/tiny/oceans.mp4";
    auto dst_uri = "/opt/ffmpeg/sample/tiny/oceans-o.mp4";

    auto demuxer = m->AddDemuxer(src_uri);
    auto muxer = m->AddMuxer(dst_uri, "mp4");
    demuxer->DumpStreams();

    for (uint32_t i = 0; i < demuxer->GetStreamNum(); i++) {
        auto src_stream = demuxer->GetStream(i);
        auto src_codecpar = src_stream->codecpar;
        if (src_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            auto src_video = FFAVNode{ src_uri, src_stream->index };
            auto dst_stream = muxer->AddStream();
            muxer->SetParams(dst_stream->index, *src_codecpar);
            auto dst_video = FFAVNode{ dst_uri, dst_stream->index };
            m->AddRule(src_video, dst_video);
        } else if (src_codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            auto src_audio = FFAVNode{ src_uri, src_stream->index };
            auto dst_stream = muxer->AddStream();
            muxer->SetParams(dst_stream->index, *src_codecpar);
            auto dst_audio = FFAVNode{ dst_uri, dst_stream->index };
            m->AddRule(src_audio, dst_audio);
        }
        PrintAVCodecParameters(src_codecpar);
    }

    m->Remux();
    muxer->DumpStreams();
}

void test_avmedia() {
    auto m = FFAVMedia::Create();

    auto src_uri = "/opt/ffmpeg/sample/tiny/oceans.mp4";
    auto dst_uri = "/opt/ffmpeg/sample/tiny/oceans-o.mp4";

    auto demuxer = m->AddDemuxer(src_uri);
    auto src_video_index = -1, src_audio_index = -1;
    for (uint32_t i = 0; i < demuxer->GetStreamNum(); i++) {
        auto src_stream = demuxer->GetStream(i);
        auto src_codecpar = src_stream->codecpar;
        if (src_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            src_video_index = src_stream->index;
        } else if (src_codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            src_audio_index = src_stream->index;
        }
        PrintAVCodecParameters(src_codecpar);
    }
    m->DumpStreams(src_uri);

    auto muxer = m->AddMuxer(dst_uri, "mp4");
    if (src_video_index >= 0) {
        auto src_video = FFAVNode{ src_uri, src_video_index };

        AVCodecParameters dst_video_params{};
        dst_video_params.codec_type = AVMEDIA_TYPE_VIDEO;
        dst_video_params.codec_id = AV_CODEC_ID_H264;
        dst_video_params.format = AV_PIX_FMT_YUV420P;
        dst_video_params.bit_rate = 4000000;
        dst_video_params.width = 960;
        dst_video_params.height = 400;
        //dst_video_params.color_range = AVCOL_RANGE_MPEG;
        //dst_video_params.color_primaries = AVCOL_PRI_BT709;
        //dst_video_params.color_trc = AVCOL_TRC_BT709;
        //dst_video_params.color_space = AVCOL_SPC_BT709;
        //dst_video_params.chroma_location = AVCHROMA_LOC_LEFT;
        dst_video_params.sample_aspect_ratio = { 1, 1 };
        //dst_video_params.video_delay = 1;
        dst_video_params.framerate = { 30, 1 };
        AVRational dst_time_base = { 1, 30 };
        auto video = muxer->AddStream(dst_video_params.codec_id);
        auto encoder = muxer->GetEncoder(video->index);
        encoder->SetGopSize(50);
        encoder->SetMaxBFrames(1);
        muxer->SetParams(video->index, dst_video_params);
        muxer->SetTimeBase(video->index, dst_time_base);

        auto dst_video = FFAVNode{ dst_uri, video->index };
        m->AddRule(src_video, dst_video);
    }
    if (src_audio_index >= 100) {
        auto src_audio = FFAVNode{ src_uri, src_audio_index };

        AVCodecParameters dst_audio_params{};
        dst_audio_params.codec_type = AVMEDIA_TYPE_AUDIO;
        dst_audio_params.codec_id = AV_CODEC_ID_AAC;
        dst_audio_params.bit_rate = 102400;
        dst_audio_params.format = AV_SAMPLE_FMT_U8;
        dst_audio_params.ch_layout = AV_CHANNEL_LAYOUT_MONO;
        dst_audio_params.sample_rate = 48000;
        AVRational dst_time_base = { 1, 48000 };
        auto audio = muxer->AddStream(dst_audio_params.codec_id);
        muxer->SetTimeBase(audio->index, dst_time_base);
        muxer->SetParams(audio->index, dst_audio_params);

        auto dst_audio = FFAVNode{ dst_uri, audio->index };
        m->AddRule(src_audio, dst_audio);
    }

    m->Transcode();
    m->DumpStreams(dst_uri);
}

int main() {
    try {
        //test_avcodec();
        //test_avformat();
        test_remux();
        //test_avmedia();
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred." << std::endl;
        return 1;
    }
    return 0;
}
