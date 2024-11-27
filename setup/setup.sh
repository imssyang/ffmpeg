#!/bin/bash

APP=ffmpeg
HOME=/opt/$APP

init() {
  apt-get install -y \
    libfontconfig1-dev \
    libwebp-dev \
    libx264-dev \
    libx265-dev

  chown -R root:root $HOME
  chmod 755 $HOME

  patchelf --set-rpath '/opt/ffmpeg/lib' ${HOME}/bin/ffmpeg
  patchelf --set-rpath '/opt/ffmpeg/lib' ${HOME}/bin/ffprobe
  patchelf --set-rpath '/opt/ffmpeg/lib' ${HOME}/bin/ffplay
}

deinit() {
  echo
}

case "$1" in
  init) init ;;
  deinit) deinit ;;
  *) SCRIPTNAME="${0##*/}"
    echo "Usage: $SCRIPTNAME {init|deinit}"
    exit 3
    ;;
esac

exit 0

# vim: syntax=sh ts=4 sw=4 sts=4 sr noet
