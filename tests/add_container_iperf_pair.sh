#!/bin/bash

#
# Adds a container running iperf -s
# and a container running iperf -c 
# running an indefinite traffic flow
#

[ -n "$TARGET_CPU" ] && {
	CPU_FLAG="--cpuset-cpus=${TARGET_CPU}-${TARGET_CPU}"
}
NEW_SERVER=`docker run -itd chrismisa/contools:iperf -s`
SERVER_IP=`docker inspect $NEW_SERVER -f '{{.NetworkSettings.IPAddress}}'`

docker run -itd $CPU_FLAG \
	chrismisa/contools:iperf -t 0 -c $SERVER_IP > /dev/null
