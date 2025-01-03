cmake_minimum_required(VERSION 3.10)

project(FFmpegCPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(FFMPEG_INCLUDE_DIR /opt/ffmpeg/include)
    set(FFMPEG_LIB_DIR /opt/ffmpeg/lib)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER clang)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(FFMPEG_INCLUDE_DIR /opt/ffmpeg/include)
    set(FFMPEG_LIB_DIR /opt/ffmpeg/lib)
    set(CMAKE_CXX_COMPILER g++)
    set(CMAKE_C_COMPILER gcc)
else()
    message(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

include_directories(${FFMPEG_INCLUDE_DIR})
link_directories(${FFMPEG_LIB_DIR})

set(SRCS
    main.cpp
    avmedia.cpp
    avformat.cpp
    avcodec.cpp
    avutil.cpp
    swscale.cpp
    swresample.cpp
)

add_executable(main ${SRCS})

target_link_libraries(main
    avcodec
    avformat
    avutil
    swscale
    swresample
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set_target_properties(main PROPERTIES
        INSTALL_RPATH ${FFMPEG_LIB_DIR}
    )
endif()

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeClean.cmake
    COMMAND ${CMAKE_COMMAND} --build . --target clean
)
