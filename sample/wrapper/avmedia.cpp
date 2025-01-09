#include <unordered_set>
#include "avmedia.h"

std::shared_ptr<FFAVMedia> FFAVMedia::Create() {
    auto instance = std::shared_ptr<FFAVMedia>(new FFAVMedia());
    if (!instance->initialize())
        return nullptr;
    return instance;
}

bool FFAVMedia::initialize() {
    return true;
}

bool FFAVMedia::writePacket(const std::string& uri, std::shared_ptr<AVPacket> packet) {
    auto muxer = GetMuxer(uri);
    if (!muxer)
        return false;

    if (!muxer->AllowMux())
        return false;

    if (!muxer->WritePacket(packet))
        return false;
    return true;
}

bool FFAVMedia::writeFrame(const std::string& uri, int stream_index, std::shared_ptr<AVFrame> frame) {
    auto muxer = GetMuxer(uri);
    if (!muxer)
        return false;

    if (!muxer->AllowMux())
        return false;

    if (!muxer->WriteFrame(stream_index, frame))
        return false;
    return true;
}

std::shared_ptr<FFAVDemuxer> FFAVMedia::GetDemuxer(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return demuxers_.count(uri) ? demuxers_.at(uri) : nullptr;
}

std::shared_ptr<FFAVMuxer> FFAVMedia::GetMuxer(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return muxers_.count(uri) ? muxers_.at(uri) : nullptr;
}

void FFAVMedia::DumpStreams(const std::string& uri) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto demuxer = GetDemuxer(uri);
    if (demuxer)
        demuxer->DumpStreams();

    auto muxer = GetMuxer(uri);
    if (muxer)
        muxer->DumpStreams();
}

std::shared_ptr<FFAVDemuxer> FFAVMedia::AddDemuxer(const std::string& uri) {
    auto demuxer = FFAVDemuxer::Create(uri);
    if (!demuxer)
        return nullptr;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    demuxers_[uri] = demuxer;
    return demuxer;
}

std::shared_ptr<FFAVMuxer> FFAVMedia::AddMuxer(const std::string& uri, const std::string& mux_fmt) {
    auto muxer = FFAVMuxer::Create(uri, mux_fmt);
    if (!muxer)
        return nullptr;

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    muxers_[uri] = muxer;
    return muxer;
}

bool FFAVMedia::AddRule(const FFAVNode& src, const FFAVNode& dst) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!rules_.count(src.uri))
        rules_[src.uri] = {};
    rules_[src.uri][src.stream_index] = dst;
    return true;
}

bool FFAVMedia::SetOption(const FFAVOption& opt) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!options_.count(opt.uri))
        options_[opt.uri] = {};
    options_[opt.uri][opt.stream_index] = opt;
    return true;
}

bool FFAVMedia::Remux() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (demuxers_.empty() || muxers_.empty() || rules_.empty())
        return false;

    bool has_packet = false;
    int src_pts_start = 0;
    std::unordered_set<std::string> src_eofs;
    std::unordered_set<std::string> dst_uris;
    while (src_eofs.size() != rules_.size()) {
        for (const auto& [uri, rules] : rules_) {
            if (src_eofs.count(uri))
                continue;

            auto demuxer = GetDemuxer(uri);
            if (!demuxer)
                return false;

            auto packet = demuxer->ReadPacket();
            if (!packet) {
                if (demuxer->ReachedEOF()) {
                    src_eofs.insert(uri);
                    continue;
                }
                return false;
            }

            if (!rules.count(packet->stream_index))
                continue;

            PrintAVPacket(packet.get());
            const auto& target = rules.at(packet->stream_index);
            auto muxer = GetMuxer(target.uri);
            if (!muxer)
                return false;

            if (!has_packet) {
                has_packet = true;
                src_pts_start = packet->pts;
            }

            auto src_dts_offset = packet->dts > packet->pts ? packet->dts - packet->pts : 0;
            auto src_stream = demuxer->GetDemuxStream(packet->stream_index)->GetStream();
            auto dst_stream = muxer->GetMuxStream(target.stream_index)->GetStream();
            std::cout << packet->pts - src_pts_start << src_dts_offset << std::endl;
            packet->pts = av_rescale_q_rnd(
                packet->pts - src_pts_start,
                src_stream->time_base,
                dst_stream->time_base,
                static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet->dts = av_rescale_q_rnd(
                packet->dts - src_pts_start - src_dts_offset,
                src_stream->time_base,
                dst_stream->time_base,
                static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet->duration = av_rescale_q(packet->duration, src_stream->time_base, dst_stream->time_base);
            packet->stream_index = target.stream_index;
            PrintAVPacket(packet.get());
            if (!writePacket(target.uri, packet))
                return false;

            if (!dst_uris.count(target.uri))
                dst_uris.insert(target.uri);
        }
    }

    for (const auto& uri : dst_uris) {
        auto muxer = GetMuxer(uri);
        if (!muxer)
            return false;

        if (!muxer->VerifyMux())
            return false;
    }

    return true;
}

bool FFAVMedia::Transcode() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (demuxers_.empty() || muxers_.empty() || rules_.empty())
        return false;

    for (int i = 0; i < 500; i++) {
        for (const auto& [uri, rules] : rules_) {
            auto demuxer = GetDemuxer(uri);
            if (!demuxer) {
                return false;
            }

            auto packet = demuxer->ReadPacket();
            if (packet) {
                if (!rules.count(packet->stream_index))
                    continue;
                //PrintAVPacket(packet.get());

                auto decode_stream = demuxer->GetDecodeStream(packet->stream_index);
                if (!decode_stream)
                    continue;

                auto frame = decode_stream->ReadFrame(packet);
                if (!frame)
                    continue;

                //PrintAVFrame(frame.get(), packet->stream_index);
                const auto& target = rules.at(packet->stream_index);
                if (!writeFrame(target.uri, target.stream_index, frame))
                    return false;
            } else {
                for (const auto& [stream_index, target] : rules) {
                    auto decode_stream = demuxer->GetDecodeStream(stream_index);
                    if (!decode_stream)
                        continue;

                    auto frame = decode_stream->ReadFrame();
                    if (!frame)
                        continue;

                    //PrintAVFrame(frame.get(), stream_index);
                    if (!writeFrame(target.uri, target.stream_index, frame))
                        return false;
                }
            }
        }
    }
    return true;
}
