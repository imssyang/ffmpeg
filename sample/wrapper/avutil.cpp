#include "avutil.h"

std::string AVError2Str(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, errnum);
    return std::string(errbuf);
}
