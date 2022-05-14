#!/bin/bash

export FFMPEG_DISABLE_ENV=yes

eval "optbin -s /opt/ffmpeg/bin"
eval "optman -s /opt/ffmpeg/share/man"
eval "optpkg -s /opt/ffmpeg/lib/pkgconfig"
eval "optlib -s /opt/ffmpeg/lib"

alias ffmpeg="ffmpeg -hide_banner"

