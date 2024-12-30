#include "avmedia.h"

std::shared_ptr<FFAVMedia> FFAVMedia::Create(const MediaParam& param) {
    auto instance = std::make_shared<FFAVMedia>();
    if (!instance->initialize(param))
        return nullptr;
    return instance;
}

bool FFAVMedia::initialize(const MediaParam& param) {
    if (param.origins.empty())
        return false;

    for (auto& origin : param.origins) {
        auto& url = origin.url;
        auto format = FFAVFormat::Load(url);
        if (!format)
            return false;
        inputs_[url] = format;
    }

    for (auto& target : param.targets) {
        auto& url = target.url;
        auto format = FFAVFormat::Create(url, target.muxer.format);
        if (!format)
            return false;
        inputs_[url] = format;
    }

    return true;
}

