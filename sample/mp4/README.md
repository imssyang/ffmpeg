# nginx

```
https://www.howtoforge.com/tutorial/how-to-build-nginx-from-source-on-ubuntu-1804-lts/
https://docs.nginx.com/nginx/admin-guide/installing-nginx/installing-nginx-open-source/#sources
http://nginx.org/en/docs/

nginx -t -c nginx.conf  检查配置文件语法
```

### 本地DNS

```
C:\Windows\System32\drivers\etc\hosts
192.168.5.220    imssyang.com
192.168.5.220    www.imssyang.com
192.168.5.220    gitea.imssyang.com
192.168.5.220    httpd.imssyang.com
192.168.5.220    nginx.imssyang.com
192.168.5.220    pgadmin.imssyang.com
192.168.5.220    freeswitch.imssyang.com
ipconfig /flushdns
```

## 自适应比特率视频（adaptive bitrate video）

[HLS vs DASH](https://www.vidbeo.com/blog/hls-vs-dash)

### HTTP Live Streaming (HLS)

[Apple HTTP Live Streaming](https://developer.apple.com/streaming)

- HTTP 
- H264 
- MPEG-2 
- 10s.ts | 10s.mp4
- manifest.m3u8`

ffmpeg -re -i bbb_sunflower_1080p_60fps_normal.mp4 -vcodec copy -loop -1 -c:a aac -b:a 160k -ar 44100 -strict -2 -f flv rtmp:192.168.5.5/live/bbb

### Smooth Streaming

[Microsoft Smooth Streaming](#)

### HDS 

[Adobe](#)

### Dynamic Adaptive Streaming over HTTP (DASH)

- HTTP
- H264|H265|VP9
- 3s.mp4
- manifest.mpd (media presentation description)

## CDN (content delivery network)


## TEST

https://dash.akamaized.net/akamai/bbb/

```shell
❯ ffprobe bbb_sunflower_1080p_60fps_normal.mp4
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'bbb_sunflower_1080p_60fps_normal.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 1
    compatible_brands: isomavc1
    creation_time   : 2013-12-16T17:59:32.000000Z
    title           : Big Buck Bunny, Sunflower version
    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013
    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net
    genre           : Animation
    composer        : Sacha Goedegebure
  Duration: 00:10:34.53, start: 0.000000, bitrate: 4486 kb/s
    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080 [SAR 1:1 DAR 16:9], 4001 kb/s, 60 fps, 60 tbr, 60k tbn, 120 tbc (default)
    Metadata:
      creation_time   : 2013-12-16T17:59:32.000000Z
      handler_name    : GPAC ISO Video Handler
    Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)
    Metadata:
      creation_time   : 2013-12-16T17:59:37.000000Z
      handler_name    : GPAC ISO Audio Handler
    Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)
    Metadata:
      creation_time   : 2013-12-16T17:59:37.000000Z
      handler_name    : GPAC ISO Audio Handler
    Side data:
      audio service type: main

❯ ffprobe BigBuckBunny_320x180.mp4
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'BigBuckBunny_320x180.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 512
    compatible_brands: mp41
    creation_time   : 1970-01-01T00:00:00.000000Z
    title           : Big Buck Bunny
    artist          : Blender Foundation
    composer        : Blender Foundation
    date            : 2008
    encoder         : Lavf52.14.0
  Duration: 00:09:56.46, start: 0.000000, bitrate: 867 kb/s
    Stream #0:0(und): Video: h264 (Constrained Baseline) (avc1 / 0x31637661), yuv420p, 320x180 [SAR 1:1 DAR 16:9], 702 kb/s, 24 fps, 24 tbr, 24 tbn, 48 tbc (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : VideoHandler
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 159 kb/s (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : SoundHandler
```

# 推流生成hls录像

```shell
❯ ffmpeg -re -i BigBuckBunny_320x180.mp4 -vcodec copy -loop -1 -c:a aac -b:a 160k -ar 44100 -strict -2 -f flv rtmp:192.168.5.5/hls/bbb
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'BigBuckBunny_320x180.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 512
    compatible_brands: mp41
    creation_time   : 1970-01-01T00:00:00.000000Z
    title           : Big Buck Bunny
    artist          : Blender Foundation
    composer        : Blender Foundation
    date            : 2008
    encoder         : Lavf52.14.0
  Duration: 00:09:56.46, start: 0.000000, bitrate: 867 kb/s
    Stream #0:0(und): Video: h264 (Constrained Baseline) (avc1 / 0x31637661), yuv420p, 320x180 [SAR 1:1 DAR 16:9], 702 kb/s, 24 fps, 24 tbr, 24 tbn, 48 tbc (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : VideoHandler
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 159 kb/s (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : SoundHandler
Stream mapping:
  Stream #0:0 -> #0:0 (copy)
  Stream #0:1 -> #0:1 (aac (native) -> aac (native))
Press [q] to stop, [?] for help
Output #0, flv, to 'rtmp:192.168.5.5/hls/bbb':
  Metadata:
    major_brand     : isom
    minor_version   : 512
    compatible_brands: mp41
    date            : 2008
    title           : Big Buck Bunny
    artist          : Blender Foundation
    composer        : Blender Foundation
    encoder         : Lavf58.29.100
    Stream #0:0(und): Video: h264 (Constrained Baseline) ([7][0][0][0] / 0x0007), yuv420p, 320x180 [SAR 1:1 DAR 16:9], q=2-31, 702 kb/s, 24 fps, 24 tbr, 1k tbn, 24 tbc (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : VideoHandler
    Stream #0:1(und): Audio: aac (LC) ([10][0][0][0] / 0x000A), 44100 Hz, stereo, fltp, 160 kb/s (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : SoundHandler
      encoder         : Lavc58.54.100 aac
[flv @ 0x556d1ff1db00] Failed to update header with correct duration.879.6kbits/s speed=   1x     
[flv @ 0x556d1ff1db00] Failed to update header with correct filesize.
frame= 1812 fps= 24 q=-1.0 Lsize=    8110kB time=00:01:15.50 bitrate= 879.9kbits/s speed=   1x    
video:6541kB audio:1480kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: 1.121525%
[aac @ 0x556d1fee4c80] Qavg: 159.677
Exiting normally, received signal 2.
```

# 推流生成dash录像

```shell
❯ ffmpeg -re -i BigBuckBunny_320x180.mp4 -c copy -f flv rtmp:192.168.5.5/dash/bbb
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'BigBuckBunny_320x180.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 512
    compatible_brands: mp41
    creation_time   : 1970-01-01T00:00:00.000000Z
    title           : Big Buck Bunny
    artist          : Blender Foundation
    composer        : Blender Foundation
    date            : 2008
    encoder         : Lavf52.14.0
  Duration: 00:09:56.46, start: 0.000000, bitrate: 867 kb/s
    Stream #0:0(und): Video: h264 (Constrained Baseline) (avc1 / 0x31637661), yuv420p, 320x180 [SAR 1:1 DAR 16:9], 702 kb/s, 24 fps, 24 tbr, 24 tbn, 48 tbc (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : VideoHandler
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 159 kb/s (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : SoundHandler
Output #0, flv, to 'rtmp:192.168.5.5/dash/bbb':
  Metadata:
    major_brand     : isom
    minor_version   : 512
    compatible_brands: mp41
    date            : 2008
    title           : Big Buck Bunny
    artist          : Blender Foundation
    composer        : Blender Foundation
    encoder         : Lavf58.29.100
    Stream #0:0(und): Video: h264 (Constrained Baseline) ([7][0][0][0] / 0x0007), yuv420p, 320x180 [SAR 1:1 DAR 16:9], q=2-31, 702 kb/s, 24 fps, 24 tbr, 1k tbn, 24 tbc (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : VideoHandler
    Stream #0:1(und): Audio: aac (LC) ([10][0][0][0] / 0x000A), 48000 Hz, stereo, fltp, 159 kb/s (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : SoundHandler
Stream mapping:
  Stream #0:0 -> #0:0 (copy)
  Stream #0:1 -> #0:1 (copy)
Press [q] to stop, [?] for help
[flv @ 0x564f738b43c0] Failed to update header with correct duration.887.7kbits/s speed=   1x     
[flv @ 0x564f738b43c0] Failed to update header with correct filesize.
frame= 1474 fps= 24 q=-1.0 Lsize=    6648kB time=00:01:01.37 bitrate= 887.3kbits/s speed=   1x    
video:5372kB audio:1198kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: 1.173738%
Exiting normally, received signal 2.
```

## mp4 to hls

http://ffmpeg.org/ffmpeg-all.html#hls-2

```shell
Default list size while converting to HLS is 5. So, you are getting the last 5 .ts files. You must set -hls_list_size 0 to include all the generated .ts files.

ffmpeg -i input.mp4 -profile:v baseline -level 3.0 -s 640x360 -start_number 0 -hls_time 10 -hls_list_size 0 -f hls index.m3u8 

ffmpeg -i input.mp4 -profile:v baseline -level 3.0 -s 640x360 -start_number 0 -hls_time 10 -hls_list_size 0 -hls_playlist_type vod -hls_segment_type fmp4 -hls_segment_filename "fseq%d.m4s" -f hls index.m3u8 

ffmpeg -y \
  -i sintel_trailer-1080p.mp4 \
  -force_key_frames "expr:gte(t,n_forced*2)" \
  -sc_threshold 0 \
  -s 1280x720 \
  -c:v libx264 -b:v 1500k \
  -c:a copy \
  -hls_time 6 \
  -hls_playlist_type vod \
  -hls_segment_type fmp4 \
  -hls_segment_filename "fileSequence%d.m4s" \
  prog_index.m3u8
```

## hls to mp4

```shell
ffmpeg -i http://10.23.24.245:880/hls/bbb/index.m3u8 -acodec copy -bsf:a aac_adtstoasc -vcodec copy out.mp4
ffmpeg -i http://10.23.24.245:880/hls/bbb/index.m3u8 -y -vcodec copy -c copy -bsf:a aac_adtstoasc out.mp4
ffmpeg -re -y -i http://10.23.24.245:880/hls/bbb/index.m3u8 -map 0 -vcodec copy -c copy -bsf:a aac_adtstoasc out.mp4
```

## split to multiple resolutions

https://ottverse.com/hls-packaging-using-ffmpeg-live-vod/

```shell
ffmpeg -i bbb_sunflower_1080p_60fps_normal.mp4 -filter_complex "[0:v]split=3[v1][v2][v3];[v1]copy[v1out];[v2]scale=w=1280:h=720[v2out];[v3]scale=w=640:h=360[v3out]"
```

## vmaf

```shell
ffmpeg -i FireHell-mpeg4-24fps.mp4 -i FireHell-mpeg4-24fps.mp4 -lavfi libvmaf="model_path=/opt/vmaf/model/vmaf_v0.6.1.json" -an -f null -
```

