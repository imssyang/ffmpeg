#pragma once

#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include "avformat.h"

class FFAVMedia {
    using FFAVFormatMap = std::unordered_map<std::string, std::shared_ptr<FFAVFormat>>;

public:
    static std::shared_ptr<FFAVMedia> Create(
        const std::vector<std::string>& input_urls,
        const std::vector<std::string>& output_urls,
        const std::vector<std::string>& output_formats);

private:
    FFAVMedia() = default;
    bool initialize(
        const std::vector<std::string>& input_urls,
        const std::vector<std::string>& output_urls,
        const std::vector<std::string>& output_formats);

private:
    FFAVFormatMap inputs_;
    FFAVFormatMap outputs_;
    mutable std::mutex mutex_;
};
