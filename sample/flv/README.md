
# 分析flv文件

```
❯ ffprobe h264-mp3.flv
Input #0, flv, from 'h264-mp3.flv':
  Duration: 00:02:09.08, start: 0.080000, bitrate: 267 kb/s
    Stream #0:0: Video: h264 (High), yuv420p(progressive), 400x304 [SAR 4:1 DAR 100:19], 200 kb/s, 25 fps, 25 tbr, 1k tbn, 50 tbc
    Stream #0:1: Audio: mp3, 44100 Hz, stereo, fltp, 64 kb/s

❯ ffprobe vp6-mp3.flv
Input #0, flv, from 'vp6-mp3.flv':
  Metadata:
    audiodelay      : 0
    canSeekToEnd    : true
  Duration: 00:00:06.00, start: 0.000000, bitrate: 118 kb/s
    Stream #0:0: Audio: mp3, 44100 Hz, stereo, fltp, 98 kb/s
    Stream #0:1: Video: vp6f, yuv420p, 360x288, 409 kb/s, 10 fps, 1k tbr, 1k tbn
```

# RTMP推流

```
ffmpeg -re -i h264-mp3.flv -f flv rtmp:imssyang.com/live/bbb
```

