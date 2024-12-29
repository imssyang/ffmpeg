#include "avmedia.h"

std::shared_ptr<FFAVMedia> FFAVMedia::Create(
    const std::vector<std::string>& input_urls,
    const std::vector<std::string>& output_urls,
    const std::vector<std::string>& output_formats
) {
    auto instance = std::make_shared<FFAVMedia>();
    if (!instance->initialize(input_urls, output_urls, output_formats))
        return nullptr;
    return instance;
}

bool FFAVMedia::initialize(
    const std::vector<std::string>& input_urls,
    const std::vector<std::string>& output_urls,
    const std::vector<std::string>& output_formats
) {
    if (output_urls.size() != output_formats.size()) {
        return false;
    }

    for (auto url : input_urls) {
        auto avformat = FFAVFormat::Load(url);
        if (!avformat)
            return false;
        inputs_[url] = avformat;
    }

    for (int i = 0; i < output_urls.size(); i++) {
        auto url = output_urls[i];
        auto avformat = FFAVFormat::Create(url, output_formats[i]);
        if (!avformat)
            return false;
        outputs_[url] = avformat;
    }

    return true;
}

