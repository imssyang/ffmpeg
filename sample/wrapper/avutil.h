#include <iostream>
#include <string>

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavutil/avutil.h>
}

std::string AVError2Str(int errnum);
