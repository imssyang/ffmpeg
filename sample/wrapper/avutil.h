#pragma once

#include <iostream>
#include <string>

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
}

std::string AVError2Str(int errnum);
std::string AVChannelLayoutStr(const AVChannelLayout* ch_layout);
void PrintAVFrame(const AVFrame* frame, int stream_index = 0, bool detailed = false);
