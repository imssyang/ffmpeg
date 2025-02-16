#include "avmanage.h"

MediaManager& MediaManager::GetInstance() {
    static MediaManager instance;
    return instance;
}

std::pair<uint32_t, std::shared_ptr<FFAVMedia>> MediaManager::NewMedia() {
    auto media = FFAVMedia::Create();
    if (!media)
        return { 0, nullptr };

    medias_[++media_id_] = media;
    return { media_id_, media };
}

std::shared_ptr<FFAVMedia> MediaManager::GetMedia(uint32_t media_id) const {
    return medias_.count(media_id) ? medias_.at(media_id) : nullptr;
}
