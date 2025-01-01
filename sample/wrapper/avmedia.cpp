#include "avmedia.h"

std::shared_ptr<FFAVMedia> FFAVMedia::Create(
    const std::vector<MediaParam>& origins,
    const std::vector<MediaParam>& targets
) {
    auto instance = std::shared_ptr<FFAVMedia>(new FFAVMedia());
    if (!instance->initialize(origins, targets))
        return nullptr;
    return instance;
}

bool FFAVMedia::initialize(
    const std::vector<MediaParam>& origins,
    const std::vector<MediaParam>& targets
) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (origins.empty() && targets.empty())
        return false;

    for (auto& param : origins) {
        auto format = FFAVFormat::Create(param.uri, param.muxer.format, param.direct);
        if (!format)
            return false;
        origins_.push_back(format);
    }

    for (auto& param : targets) {
        auto format = FFAVFormat::Create(param.uri, param.muxer.format, param.direct);
        if (!format)
            return false;
        targets_.push_back(format);
    }

    return true;
}

void FFAVMedia::DumpStreams() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto format : origins_) {
        format->DumpStreams();
    }
    for (auto format : targets_) {
        format->DumpStreams();
    }
}

std::vector<int> FFAVMedia::GetStreamIDs(const std::string& uri, AVMediaType media_type) {
    auto format = GetFormat(uri);
    if (!format)
        return {};

    std::vector<int> streamIDs;
    int nb_streams = format->GetNumOfStreams();
    for (int i = 0; i < nb_streams; i++) {
        auto stream = format->GetStream(i);
        if (media_type == stream->codecpar->codec_type) {
            streamIDs.push_back(stream->id);
        }
    }
    return streamIDs;
}

std::shared_ptr<AVStream> FFAVMedia::GetStream(const std::string& uri, int stream_id) {
    auto format = GetFormat(uri);
    if (!format)
        return nullptr;
    return format->GetStreamByID(stream_id);
}

std::shared_ptr<FFAVFormat> FFAVMedia::GetFormat(const std::string& uri) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto format : origins_) {
        if (format->GetURI() == uri) {
            return format;
        }
    }
    for (auto format : targets_) {
        if (format->GetURI() == uri) {
            return format;
        }
    }
    return nullptr;
}

bool FFAVMedia::Transcode() {
    return false;
}
