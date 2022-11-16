#!/bin/bash

export FFMPEG_DISABLE_ENV=yes

eval "optbin -s /opt/ffmpeg/bin"
eval "optman -s /opt/ffmpeg/share/man"
eval "optpkg -s /opt/ffmpeg/lib/pkgconfig"
eval "optlib -s /opt/ffmpeg/lib"

alias ffmpeg="ffmpeg -hide_banner -probesize 300000000"
alias ffplay="ffplay -hide_banner -probesize 300000000 -x 1067 -y 600 -left 150 -top 150 -an -infbuf"
alias ffprobe="ffprobe -hide_banner -probesize 300000000 -sexagesimal -pretty -unit"

