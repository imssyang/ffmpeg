#!/bin/bash

do_ffmpeg() {
  local in=$1
  local out=${2:-"-f null -"}
  local job=${3:-0}
  local N=${4:-0}
  echo "ffmpeg: in[$in] out[$out] job[$job/$N]"
  /bin/time -f "job=$job/$N wall=%e user=%U sys=%S rss=%Mkb cpu=%P" \
    taskset -c $job \
    ffmpeg -y -loglevel warning -stats \
    -i $in -vn -ac 2 -b:a 128k -ar 48000 \
    -codec:a libmp3lame -threads 1 \
    $out
}

export -f do_ffmpeg

single_file() {
  for i in {1..10}; do
    echo ">>> processing $i"
    do_ffmpeg input.m4a out_$i.mp3
  done
}

multi_file() {
  for f in *.m4a; do
    echo ">>> processing $f"
    do_ffmpeg "$f" #"out_${f%.m4a}.mp3"
  done
}

concurrent_cmd() {
  ccnum=$1 #4
  ls *.m4a | xargs -n 1 -P $ccnum -I {} \
    ffmpeg -y -loglevel error \
    -i {} -vn -ac 2 -b:a 128k -ar 48000 \
    -codec:a libmp3lame {}.mp3
}

concurrent_single_file() {
  N=$1
  echo "==== concurrency = $N ===="
  seq 1 $N | xargs -P $N -I {} \
    bash -c 'do_ffmpeg "$@"' _ input.m4a "" {} "$N"
}

with_time() {
  start=$(date +%s)
  #single_file
  #multi_file
  concurrent_single_file 3
  end=$(date +%s)
  echo "Total time: $((end - start)) seconds"
}

with_time $@

