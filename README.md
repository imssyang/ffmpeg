FFmpeg
======

FFmpeg is a collection of libraries and tools to process multimedia content
such as audio, video, subtitles and related metadata.

## Libraries

* `libavcodec` provides implementation of a wider range of codecs.
* `libavformat` implements streaming protocols, container formats and basic I/O access.
* `libavutil` includes hashers, decompressors and miscellaneous utility functions.
* `libavfilter` provides a mean to alter decoded Audio and Video through chain of filters.
* `libavdevice` provides an abstraction to access capture and playback devices.
* `libswresample` implements audio mixing and resampling routines.
* `libswscale` implements color conversion and scaling routines.

## Tools

* [ffmpeg](https://ffmpeg.org/ffmpeg.html) is a command line toolbox to
  manipulate, convert and stream multimedia content.
* [ffplay](https://ffmpeg.org/ffplay.html) is a minimalistic multimedia player.
* [ffprobe](https://ffmpeg.org/ffprobe.html) is a simple analysis tool to inspect
  multimedia content.
* Additional small tools such as `aviocat`, `ismindex` and `qt-faststart`.

## Documentation

The offline documentation is available in the **doc/** directory.

The online documentation is available in the main [website](https://ffmpeg.org)
and in the [wiki](https://trac.ffmpeg.org).

## Compile

### Windows

```bash
./configure --enable-debug --enable-rpath --enable-shared --disable-static \
    --enable-debug --extra-cflags=-g --extra-ldflags=-g --prefix=_build \
    --disable-asm --disable-everything --disable-logging --disable-runtime-cpudetect \
    --disable-doc --disable-htmlpages --disable-manpages  --disable-podpages  --disable-txtpages \
    --disable-encoders \
    --disable-decoders \
    --enable-decoder=aac --enable-decoder=mp3 --enable-decoder=mp2 \
    --enable-decoder=hevc --enable-decoder=h264 --enable-decoder=mpeg4 \
    --enable-decoder=pcm_mulaw --enable-decoder=pcm_alaw \
    --enable-decoder=g723_1 --enable-decoder=adpcm_g726  --enable-decoder=adpcm_g722 \
    --enable-decoder=g729 --enable-decoder=amrnb --enable-decoder=opus \
    --disable-muxers --enable-muxer=mp4 \
    --disable-demuxers --enable-demuxer=mov --enable-demuxer=h264 \
    --disable-bsfs  \
    --disable-parsers \
    --disable-filters \
    --disable-indevs \
    --disable-outdevs \
    --disable-avdevice --disable-avfilter --disable-swresample \
    --disable-swscale --disable-swscale-alpha --disable-devices --disable-hwaccels \
    --disable-protocols --enable-protocol=file \
    --disable-ffmpeg --disable-ffprobe --disable-ffplay \
    --enable-bsf=h264_mp4toannexb --enable-bsf=svac_mp4toannexb \
    --arch=x86_32 --toolchain=msvc
```

### Linux

```bash
apt install \
  libfontconfig1-dev \
  libsdl2-dev \
  libwebp-dev \
  libx264-dev \
  libx265-dev

./configure --enable-debug --enable-rpath --enable-shared --disable-static \
    --enable-debug --extra-cflags=-g --extra-ldflags=-g --prefix=buildme \
    --disable-asm --disable-everything --disable-logging --disable-runtime-cpudetect \
    --disable-doc --disable-htmlpages --disable-manpages  --disable-podpages  --disable-txtpages \
    --disable-encoders \
    --disable-decoders \
    --enable-decoder=aac --enable-decoder=mp3 --enable-decoder=mp2 \
    --enable-decoder=hevc --enable-decoder=h264 --enable-decoder=mpeg4 \
    --enable-decoder=pcm_mulaw --enable-decoder=pcm_alaw \
    --enable-decoder=g723_1 --enable-decoder=adpcm_g726  --enable-decoder=adpcm_g722 \
    --enable-decoder=g729 --enable-decoder=amrnb --enable-decoder=opus \
    --disable-muxers --enable-muxer=mp4 \
    --disable-demuxers --enable-demuxer=mov --enable-demuxer=h264 \
    --disable-bsfs  \
    --disable-parsers \
    --disable-filters \
    --disable-indevs \
    --disable-outdevs \
    --disable-avdevice --disable-avfilter --disable-swresample \
    --disable-swscale --disable-swscale-alpha --disable-devices --disable-hwaccels \
    --disable-protocols --enable-protocol=file \
    --enable-bsf=h264_mp4toannexb --enable-bsf=svac_mp4toannexb \
    --disable-ffmpeg --disable-ffprobe --disable-ffplay

# (场景1) linux完全构建
./configure --prefix=/opt/ffmpeg \
    --enable-gpl \
    --enable-version3 \
    --enable-nonfree \
    --enable-gray \
    --enable-iconv \
    --enable-rpath \
    --enable-sdl2 \
    --enable-shared \
    --enable-openssl \
    --enable-libfontconfig \
    --enable-libfreetype \
    --enable-libmp3lame \
    --enable-libopus \
    --enable-libspeex \
    --enable-libwebp \
    --enable-libx264 \
    --enable-libx265 \
    --enable-libxml2 \
    --enable-libvmaf \
    --enable-bsf=aac_adtstoasc \
    --enable-bsf=h264_metadata \
    --enable-bsf=h264_mp4toannexb \
    --enable-bsf=h264_redundant_pps \
    --enable-bsf=hevc_metadata \
    --enable-bsf=hevc_mp4toannexb \
    --enable-bsf=mov2textsub \
    --enable-bsf=mpeg4_unpack_bframes \
    --enable-bsf=noise \
    --enable-bsf=null \
    --enable-bsf=remove_extradata \
    --enable-bsf=text2movsub \
    --enable-bsf=trace_headers \
    --enable-bsf=vp9_metadata \
    --enable-bsf=vp9_raw_reorder \
    --enable-bsf=vp9_superframe \
    --enable-bsf=vp9_superframe_split \
    --enable-encoder=aac \
    --enable-encoder=adpcm_g722 \
    --enable-encoder=adpcm_g726 \
    --enable-encoder=flac \
    --enable-encoder=flv \
    --enable-encoder=h263 \
    --enable-encoder=h264_v4l2m2m \
    --enable-encoder=hevc_v4l2m2m \
    --enable-encoder=opus \
    --enable-encoder=pcm_alaw \
    --enable-encoder=pcm_mulaw \
    --enable-encoder=png \
    --enable-encoder=vp8_v4l2m2m \
    --enable-decoder=aac \
    --enable-decoder=adpcm_g722 \
    --enable-decoder=adpcm_g726 \
    --enable-decoder=flac \
    --enable-decoder=g723_1 \
    --enable-decoder=g729 \
    --enable-decoder=mp3 \
    --enable-decoder=mpeg4 \
    --enable-decoder=h264 \
    --enable-decoder=hevc \
    --enable-decoder=opus \
    --enable-decoder=pcm_mulaw \
    --enable-decoder=pcm_alaw \
    --enable-decoder=vp6 \
    --enable-decoder=vp8 \
    --enable-decoder=vp9 \
    --enable-demuxer=aac \
    --enable-demuxer=ape \
    --enable-demuxer=avi \
    --enable-demuxer=flac \
    --enable-demuxer=flv \
    --enable-demuxer=g722 \
    --enable-demuxer=g729 \
    --enable-demuxer=gif \
    --enable-demuxer=h263 \
    --enable-demuxer=h264 \
    --enable-demuxer=hevc \
    --enable-demuxer=hls \
    --enable-demuxer=m4v \
    --enable-demuxer=mov \
    --enable-demuxer=mp3 \
    --enable-demuxer=ogg \
    --enable-demuxer=pcm_alaw \
    --enable-demuxer=pcm_mulaw \
    --enable-demuxer=rtp \
    --enable-demuxer=rtsp \
    --enable-demuxer=wav \
    --enable-muxer=aptx \
    --enable-muxer=aptx_hd \
    --enable-muxer=avi \
    --enable-muxer=dash \
    --enable-muxer=dts \
    --enable-muxer=f4v \
    --enable-muxer=flac \
    --enable-muxer=flv \
    --enable-muxer=g722 \
    --enable-muxer=g726 \
    --enable-muxer=gif \
    --enable-muxer=h264 \
    --enable-muxer=hls \
    --enable-muxer=m4v \
    --enable-muxer=mov \
    --enable-muxer=mp3 \
    --enable-muxer=mp4 \
    --enable-muxer=ogg \
    --enable-muxer=opus \
    --enable-muxer=wav \
    --enable-filter=acopy \
    --enable-filter=aformat \
    --enable-filter=color \
    --enable-filter=colorbalance \
    --enable-filter=colorchannelmixer \
    --enable-filter=colorhold \
    --enable-filter=colorkey \
    --enable-filter=colorlevels \
    --enable-filter=colorspace \
    --enable-filter=copy \
    --enable-filter=fps \
    --enable-filter=gblur \
    --enable-filter=loop \
    --enable-filter=noise \
    --enable-filter=null \
    --enable-filter=overlay \
    --enable-filter=reverse \
    --enable-filter=scale \
    --enable-indev=alsa \
    --enable-outdev=alsa \
    --enable-outdev=sdl2 \
    --enable-protocol=async \
    --enable-protocol=file \
    --enable-protocol=ftp \
    --enable-protocol=hls \
    --enable-protocol=http \
    --enable-protocol=httpproxy \
    --enable-protocol=https \
    --enable-protocol=rtmp \
    --enable-protocol=rtmps \
    --enable-protocol=rtmpt \
    --enable-protocol=rtmpts \
    --enable-protocol=rtp \
    --enable-protocol=srtp \
    --enable-protocol=tcp \
    --enable-protocol=tls \
    --enable-protocol=udp \
    --enable-protocol=unix


    --datadir=/opt/ffmpeg/data \
    --docdir=/opt/ffmpeg/doc \
    --mandir=/opt/ffmpeg/man \
    --enable-librtmp \
    --enable-libopencv \
    --extra-cflags="-I/opt/opencv/include" \
    --extra-ldflags="-L/opt/opencv/lib" \
    --extra-libs="-lopencv_core"

./configure --prefix=/opt/ffmpeg \
    --datadir=/opt/ffmpeg/data \
    --docdir=/opt/ffmpeg/doc \
    --mandir=/opt/ffmpeg/man \
    --enable-debug \
    --enable-gpl \
    --enable-version3 \
    --enable-gnutls \
    --enable-gray \
    --enable-iconv \
    --enable-rpath \
    --enable-sdl2 \
    --enable-shared \
    --enable-avisynth \
    --enable-hardcoded-tables \
    --enable-libfontconfig \
    --enable-libfreetype \
    --enable-libmp3lame \
    --enable-libopus \
    --enable-libspeex \
    --enable-libxml2 \
    --enable-pic
```

### Mac

```bash
brew install pkg-config \
  automake \
  libass \
  libbluray \
  libiconv \
  libmodplug \
  libvmaf \
  libvpx \
  libxml2 \
  aom \
  dav1d \
  fdk-aac \
  fontconfig \
  freetype \
  gray \
  harfbuzz \
  lame \
  openh264 \
  openjpeg \
  openssl \
  opus \
  sdl2 \
  speex \
  webp \
  x264 \
  x265 \
  zimg \
  zmq

./configure --prefix=/opt/ffmpeg \
    --enable-gpl \
    --enable-version3 \
    --enable-nonfree \
    --enable-fontconfig \
    --enable-gray \
    --enable-iconv \
    --enable-rpath \
    --enable-sdl2 \
    --enable-shared \
    --enable-openssl \
    --enable-libaom \
    --enable-libass \
    --enable-libbluray \
    --enable-libdav1d \
    --enable-libfreetype \
    --enable-libharfbuzz \
    --enable-libmodplug \
    --enable-libopenh264 \
    --enable-libopenjpeg \
    --enable-libopus \
    --enable-libspeex \
    --enable-libvmaf \
    --enable-libvpx \
    --enable-libwebp \
    --enable-libx264 \
    --enable-libx265 \
    --enable-libxml2 \
    --enable-libzimg \
    --enable-libzmq \
    --enable-bsf=aac_adtstoasc \
    --enable-bsf=h264_metadata \
    --enable-bsf=h264_mp4toannexb \
    --enable-bsf=h264_redundant_pps \
    --enable-bsf=hevc_metadata \
    --enable-bsf=hevc_mp4toannexb \
    --enable-bsf=mov2textsub \
    --enable-bsf=mpeg4_unpack_bframes \
    --enable-bsf=noise \
    --enable-bsf=null \
    --enable-bsf=remove_extradata \
    --enable-bsf=text2movsub \
    --enable-bsf=trace_headers \
    --enable-bsf=vp9_metadata \
    --enable-bsf=vp9_raw_reorder \
    --enable-bsf=vp9_superframe \
    --enable-bsf=vp9_superframe_split \
    --enable-encoder=aac \
    --enable-encoder=adpcm_g722 \
    --enable-encoder=adpcm_g726 \
    --enable-encoder=flac \
    --enable-encoder=flv \
    --enable-encoder=h263 \
    --enable-encoder=h264_v4l2m2m \
    --enable-encoder=hevc_v4l2m2m \
    --enable-encoder=opus \
    --enable-encoder=pcm_alaw \
    --enable-encoder=pcm_mulaw \
    --enable-encoder=png \
    --enable-encoder=vp8_v4l2m2m \
    --enable-decoder=aac \
    --enable-decoder=adpcm_g722 \
    --enable-decoder=adpcm_g726 \
    --enable-decoder=flac \
    --enable-decoder=g723_1 \
    --enable-decoder=g729 \
    --enable-decoder=mp3 \
    --enable-decoder=mpeg4 \
    --enable-decoder=h264 \
    --enable-decoder=hevc \
    --enable-decoder=opus \
    --enable-decoder=pcm_mulaw \
    --enable-decoder=pcm_alaw \
    --enable-decoder=vp6 \
    --enable-decoder=vp8 \
    --enable-decoder=vp9 \
    --enable-demuxer=aac \
    --enable-demuxer=ape \
    --enable-demuxer=avi \
    --enable-demuxer=flac \
    --enable-demuxer=flv \
    --enable-demuxer=g722 \
    --enable-demuxer=g729 \
    --enable-demuxer=gif \
    --enable-demuxer=h263 \
    --enable-demuxer=h264 \
    --enable-demuxer=hevc \
    --enable-demuxer=hls \
    --enable-demuxer=m4v \
    --enable-demuxer=mov \
    --enable-demuxer=mp3 \
    --enable-demuxer=ogg \
    --enable-demuxer=pcm_alaw \
    --enable-demuxer=pcm_mulaw \
    --enable-demuxer=rtp \
    --enable-demuxer=rtsp \
    --enable-demuxer=wav \
    --enable-muxer=aptx \
    --enable-muxer=aptx_hd \
    --enable-muxer=avi \
    --enable-muxer=dash \
    --enable-muxer=dts \
    --enable-muxer=f4v \
    --enable-muxer=flac \
    --enable-muxer=flv \
    --enable-muxer=g722 \
    --enable-muxer=g726 \
    --enable-muxer=gif \
    --enable-muxer=h264 \
    --enable-muxer=hls \
    --enable-muxer=m4v \
    --enable-muxer=mov \
    --enable-muxer=mp3 \
    --enable-muxer=mp4 \
    --enable-muxer=ogg \
    --enable-muxer=opus \
    --enable-muxer=wav \
    --enable-filter=acopy \
    --enable-filter=aformat \
    --enable-filter=color \
    --enable-filter=colorbalance \
    --enable-filter=colorchannelmixer \
    --enable-filter=colorhold \
    --enable-filter=colorkey \
    --enable-filter=colorlevels \
    --enable-filter=colorspace \
    --enable-filter=copy \
    --enable-filter=fps \
    --enable-filter=gblur \
    --enable-filter=loop \
    --enable-filter=noise \
    --enable-filter=null \
    --enable-filter=overlay \
    --enable-filter=reverse \
    --enable-filter=scale \
    --enable-indev=alsa \
    --enable-outdev=alsa \
    --enable-outdev=sdl2 \
    --enable-protocol=async \
    --enable-protocol=file \
    --enable-protocol=ftp \
    --enable-protocol=hls \
    --enable-protocol=http \
    --enable-protocol=httpproxy \
    --enable-protocol=https \
    --enable-protocol=rtmp \
    --enable-protocol=rtmps \
    --enable-protocol=rtmpt \
    --enable-protocol=rtmpts \
    --enable-protocol=rtp \
    --enable-protocol=srtp \
    --enable-protocol=tcp \
    --enable-protocol=tls \
    --enable-protocol=udp \
    --enable-protocol=unix \
    --enable-debug \
    --extra-cflags="-g -O0" \
    --extra-ldflags="-g"
```

