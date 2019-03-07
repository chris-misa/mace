#!/bin/bash

B="===================="

NUM_ROUNDS=3

PING_ARGS="-D -i 0.0 -s 1472 -c 2000"

TARGET="10.10.1.2"

OUTER_DEV_ID=5

NATIVE_PING_CMD="ping"
CONTAINER_PING_CMD="/iputils/ping"

PING_CONTAINER_IMAGE="chrismisa/slow-ping"
PING_CONTAINER_NAME="ping-container"

ANALYSIS_CMD="Rscript report_one_shot.r"

META_DATA="metadata"
MANIFEST="manifest"

MACE_PATH=`echo ${PWD%${PWD##*/}}`

PAUSE_CMD="sleep 5"

DATE_STR=`date +%Y%m%d%H%M%S`

mkdir $DATE_STR
pushd $DATE_STR > /dev/null

echo -e "$B uname -a $B\n $(uname -a)\n" >> $META_DATA
echo -e "$B lshw: $B\n $(lshw)\n" >> $META_DATA

echo $B Running mace one-shot test $B

echo "native_control.ping" >> $MANIFEST
echo "container_control.ping" >> $MANIFEST
echo "native_monitored.ping" >> $MANIFEST
echo "container_monitored.ping" >> $MANIFEST
echo "container_monitored.lat" >> $MANIFEST

for i in `seq 1 $NUM_ROUNDS`
do

  echo $B Round $i $B

  docker run -itd --name=$PING_CONTAINER_NAME \
                  --entrypoint=/bin/bash \
                  $PING_CONTAINER_IMAGE \
                  > /dev/null
  echo "  Ping container up"

  $PAUSE_CMD

  #
  # Native Control base-line
  #
  $NATIVE_PING_CMD $PING_ARGS $TARGET >> native_control.ping
  echo "  Took native control"

  $PAUSE_CMD

  #
  # Container Control base-line
  #
  docker exec $PING_CONTAINER_NAME \
    $CONTAINER_PING_CMD $PING_ARGS $TARGET >> container_control.ping
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
                  --device /dev/mace:/dev/mace \
                  -v /sys/class/mace:/mace \
                  --entrypoint=/bin/bash \
                  $PING_CONTAINER_IMAGE \
                  > /dev/null
  echo "  Ping container up"
  docker exec $PING_CONTAINER_NAME \
    bash -c 'echo 1 > /mace/on'
  echo "  Mace active in ping container"

  $PAUSE_CMD

  #
  # Native Monitored for perturbation
  #
  $NATIVE_PING_CMD $PING_ARGS $TARGET >> native_monitored.ping
  echo "  Took native monitored"

  $PAUSE_CMD

  #
  # Container monitored
  #
  docker exec $PING_CONTAINER_NAME \
    $CONTAINER_PING_CMD $PING_ARGS $TARGET >> container_monitored.ping
  echo "  Took container monitored"

  docker exec $PING_CONTAINER_NAME \
    cat /dev/mace >> container_monitored.lat
  echo "  Retrieved container latencies"

  $PAUSE_CMD
  docker stop $PING_CONTAINER_NAME > /dev/null
  docker rm $PING_CONTAINER_NAME > /dev/null

  #
  # Remove module
  #
  rmmod mace

  $PAUSE_CMD
done

popd > /dev/null

echo "  Generating graphs and analysis"
$ANALYSIS_CMD $DATE_STR

echo Done.
