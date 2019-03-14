#!/bin/bash

#
# Adds an extra container pinging a specified remote target
#

[ -n "$TARGET_CPU" ] && {
	CPU_FLAG="--cpuset-cpus=${TARGET_CPU}-${TARGET_CPU}"
}

docker run -itd $CPU_FLAG \
  chrismisa/slow-ping $BG_PING_ARGS > /dev/null
