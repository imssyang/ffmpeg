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
