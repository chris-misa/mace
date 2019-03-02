#!/bin/bash

B="===================="

PING_ARGS="-D -i 0.0 -s 56 -c 100"

TARGET="192.168.0.1"

OUTER_DEV_ID=2

NATIVE_PING_CMD="ping"
CONTAINER_PING_CMD="ping"

PING_CONTAINER_IMAGE="chrismisa/slim-ping"
PING_CONTAINER_NAME="ping-container"

META_DATA="metadata"
MANIFEST="manifest"

MACE_PATH=`echo ${PWD%${PWD##*/}}`

PAUSE_CMD="sleep 5"

DATE_STR=`date +%Y%m%d%H%M%S`

mkdir $DATE_STR
cd $DATE_STR

echo -e "$B uname -a $B\n $(uname -a)\n" >> $META_DATA
echo -e "$B lshw: $B\n $(lshw)\n" >> $META_DATA

echo $B Running mace one-shot test $B

docker run -itd --name=$PING_CONTAINER_NAME \
                $PING_CONTAINER_IMAGE
echo "  Ping container up"

$PAUSE_CMD

#
# Native Control base-line
#
$NATIVE_PING_CMD $PING_ARGS $TARGET > native_control.ping
echo "native_control.ping" >> $MANIFEST
echo "  Took native control"

$PAUSE_CMD

#
# Container Control base-line
#
docker exec $PING_CONTAINER_NAME \
  $CONTAINER_PING_CMD $PING_ARGS $TARGET > container_control.ping
echo "container_control.ping" >> $MANIFEST
echo "  Took container control"

$PAUSE_CMD
docker stop $PING_CONTAINER_NAME > /dev/null
docker rm $PING_CONTAINER_NAME > /dev/null

#
# Insert module
#
insmod ${MACE_PATH}mace.ko outer_dev=$OUTER_DEV_ID
[ $? -eq 0 ] || (echo "Failed to insert module" && exit)
echo "  Inserted module"

docker run -itd --name=$PING_CONTAINER_NAME \
                -v /sys/kernel/mace:/mace \
                $PING_CONTAINER_IMAGE
echo "  Ping container up"
docker exec $PING_CONTAINER_NAME \
  bash -c 'echo 1 > /mace/mace_on'
echo "  Mace active in ping container"

$PAUSE_CMD

#
# Native Monitored for perturbation
#
$NATIVE_PING_CMD $PING_ARGS $TARGET > native_monitored.ping
echo "native_monitored.ping" >> $MANIFEST
echo "  Took native monitored"

$PAUSE_CMD


#
# Container monitored
#
docker exec $PING_CONTAINER_NAME \
  $CONTAINER_PING_CMD $PING_ARGS $TARGET > container_monitored.ping
echo "container_monitored.ping" >> $MANIFEST
echo "  Took container monitored"

docker exec $PING_CONTAINER_NAME \
  cat /mace/latencies_ns > container_monitored.lat
echo "container_monitored.lat" >> $MANIFEST
echo "  Retrieved container latencies"

$PAUSE_CMD
docker stop $PING_CONTAINER_NAME > /dev/null
docker rm $PING_CONTAINER_NAME > /dev/null

#
# Remove module
#
rmmod mace

echo Done.
