
ffmpeg -re -i lol-ob-1080p-60fps-aac-5m.flv -c copy -f flv rtmp:192.168.5.5/live/lol
ffmpeg -re -i lol-ob-1080p-60fps-aac-220213-blg-vs-lng.flv -c copy -f flv rtmp:192.168.5.5/live/lol
ffmpeg -stream_loop 3 -ss 400 -re -i lol-ob-1080p-60fps-aac-220213-blg-vs-lng.flv -c copy -f flv rtmp:192.168.5.5/live/lol
ffmpeg -stream_loop 3 -ss 10 -re -i lol-ob-1080p-60fps-aac-220213-blg-vs-lng.flv -c copy -f flv "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_1528640333_27984654&key=fa5e70a547ea79ca457f85a0079f2487&schedule=rtmp&pflag=1"

ffmpeg -re -i lol-zb-1080p-30fps-aac-40m.flv -c copy -f flv rtmp:192.168.5.5/live/lol
ffmpeg -stream_loop 3 -ss 400 -re -i lol-zb-1080p-30fps-aac-40m.flv -c copy -f flv rtmp:192.168.5.5/live/lol
ffmpeg -stream_loop 3 -re -i lol-ob-eng-msi-pentakill.flv -c copy -f flv rtmp:192.168.5.5/live/lol

ffmpeg -re -i lol-zb-1080p-30fps-aac-40m.flv -c copy -f flv "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_1528640333_27984654&key=fa5e70a547ea79ca457f85a0079f2487&schedule=rtmp&pflag=1"

ffmpeg -stream_loop 10 -ss 2100 -re -i 343245359_da3-1-116.flv -vcodec libx264 -acodec aac -f flv "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_1528640333_27984654&key=fa5e70a547ea79ca457f85a0079f2487&schedule=rtmp&pflag=1"

ffmpeg -stream_loop 10 -ss 1800 -re -i 343245359_da3-1-116.flv -c copy -f flv "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_32232369_74790256&key=f48877047469a274d7e87d04710f3216&schedule=rtmp&pflag=1"
ffmpeg -stream_loop -1 -re -i 5kill_7min.mp4 -vcodec libx264 -acodec aac -f flv "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_32232369_74790256&key=f48877047469a274d7e87d04710f3216&schedule=rtmp&pflag=1"
ffmpeg -stream_loop -1 -re -i 5kill_7min.mp4 -c copy -f flv "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_32232369_74790256&key=f48877047469a274d7e87d04710f3216&schedule=rtmp&pflag=1"

