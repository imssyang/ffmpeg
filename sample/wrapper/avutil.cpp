#include "avutil.h"

std::string AVError2Str(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, errnum);
    return std::string(errbuf);
}

std::string AVChannelLayoutStr(const AVChannelLayout* ch_layout) {
    char buf[64]{};
    av_channel_layout_describe(ch_layout, buf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(buf);
}

void PrintAVFrame(const AVFrame* frame, int stream_index, bool detailed) {
    if (!frame) {
        return;
    }

    bool isvideo = bool(frame->width > 0 || frame->height > 0);
    bool isaudio = bool();

    std::cout << "frame";
    if (stream_index >= 0)
        std::cout << " index:" << stream_index;
    std::cout << " pts:" << frame->pts
        << " pkt_dts:" << frame->pkt_dts
        << " duration:" << frame->duration
        << " time_base:" << frame->time_base.num << "/" << frame->time_base.den
        << " quality:" << frame->quality;

    if (isvideo) {
        std::cout << " width:" << frame->width
            << " height:" << frame->height
            << " SAR:" << frame->sample_aspect_ratio.num << "/" << frame->sample_aspect_ratio.den
            << " pix_fmt:" << av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format))
            << " pict_type:";
        switch (frame->pict_type) {
            case AV_PICTURE_TYPE_I: std::cout << "I"; break;
            case AV_PICTURE_TYPE_P: std::cout << "P"; break;
            case AV_PICTURE_TYPE_B: std::cout << "B"; break;
            default: std::cout << "-"; break;
        }

        int total_size = 0;
        std::cout << " planesize:";
        for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
            if (frame->data[i]) {
                int plane_size = av_image_get_linesize(static_cast<AVPixelFormat>(frame->format), frame->width, i) * frame->height;
                total_size += plane_size;
                std::cout << plane_size << " ";
            }
        }
        std::cout << "size:" << total_size;
        if (detailed) {
            std::cout << " color_range:" << av_color_range_name(frame->color_range)
                << " color_primaries:" << av_color_primaries_name(frame->color_primaries)
                << " color_trc:" << av_color_transfer_name(frame->color_trc)
                << " color_space:" << av_color_space_name(frame->colorspace)
                << " chroma_location:" << av_chroma_location_name(frame->chroma_location)
                << " crop:" << frame->crop_top
                << " "<< frame->crop_bottom
                << " "<< frame->crop_left
                << " "<< frame->crop_right;
        }
    }

    if (isaudio) {
        int bytes_per_sample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format));
        int64_t total_size = static_cast<int64_t>(frame->ch_layout.nb_channels) * frame->nb_samples * bytes_per_sample;
        std::cout << " sample_fmt:" << av_get_sample_fmt_name(static_cast<AVSampleFormat>(frame->format))
            << " sample_rate:" << frame->sample_rate
            << " bytes_per_sample:" << bytes_per_sample
            << " nb_samples:" << frame->nb_samples
            << " ch_layout:" << AVChannelLayoutStr(&frame->ch_layout)
            << " size:" << total_size;
    }

    std::cout << " linesize:";
    for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
        if (frame->linesize[i] > 0) {
            std::cout << frame->linesize[i] << " ";
        }
    }
    std::cout << std::endl;
}