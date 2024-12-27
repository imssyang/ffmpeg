cmake_minimum_required(VERSION 3.10)

project(demuxing)

set(CMAKE_C_STANDARD 11)
set(FFMPEG_PATH "/opt/ffmpeg")

include_directories(${FFMPEG_PATH}/include)
find_library(AVUTIL_LIBRARIES NAMES avutil PATHS ${FFMPEG_PATH}/lib)
find_library(AVCODEC_LIBRARIES NAMES avcodec PATHS ${FFMPEG_PATH}/lib)
find_library(AVFORMAT_LIBRARIES NAMES avformat PATHS ${FFMPEG_PATH}/lib)

if(NOT AVUTIL_LIBRARIES)
    message(FATAL_ERROR "Could not find avutil")
endif()

add_executable(demuxing main.cpp)

target_link_libraries(demuxing PRIVATE
    ${AVFORMAT_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
)

#target_compile_definitions(ffmpeg_demuxing PRIVATE ${AVFORMAT_CFLAGS_OTHER})
#target_link_options(ffmpeg_demuxing PRIVATE ${AVFORMAT_LDFLAGS_OTHER})

