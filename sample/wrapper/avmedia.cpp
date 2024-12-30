#include "avmedia.h"

std::shared_ptr<FFAVMedia> FFAVMedia::Create(
    const std::vector<MediaParam>& origins,
    const std::vector<MediaParam>& targets
) {
    auto instance = std::make_shared<FFAVMedia>();
    if (!instance->initialize(origins, targets))
        return nullptr;
    return instance;
}

bool FFAVMedia::initialize(
    const std::vector<MediaParam>& origins,
    const std::vector<MediaParam>& targets
) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (origins.empty() && targets.empty())
        return false;

    for (auto& param : origins) {
        auto format = FFAVFormat::Load(param.uri);
        if (!format)
            return false;
        origins_.push_back(format);
    }

    for (auto& param : targets) {
        auto format = FFAVFormat::Create(param.uri, param.muxer.format);
        if (!format)
            return false;
        targets_.push_back(format);
    }

    return true;
}

std::vector<int> FFAVMedia::GetIDs(const std::string& uri, AVMediaType media_type) {
    return {};
}

std::shared_ptr<MediaVideo> FFAVMedia::GetVideo(int id) {
    return nullptr;
}

std::shared_ptr<MediaAudio> FFAVMedia::GetAudio(int id) {
    return nullptr;
}

std::shared_ptr<AVPacket> FFAVMedia::GetPacket(AVMediaType media_type, int id) {
    return nullptr;
}

std::shared_ptr<AVFrame> FFAVMedia::GetFrame(AVMediaType media_type, int id) {
    return nullptr;
}

bool FFAVMedia::Seek(const std::string& uri, AVMediaType media_type, int id, int64_t timestamp) {
    return false;
}

bool FFAVMedia::Transcode() {
    return false;
}
