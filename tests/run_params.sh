#!/bin/bash

#
# Measure the effect of ping parameters on RTT difference between container and native contexts
#

export B="===================="

export NUM_ROUNDS=1

export PING_ARGS="-D -i 0.0 -c 2000"

export SIZES="`seq 16 100 1472`"

export TARGET="10.10.1.2"

export OUTER_DEV_ID=3
export OUTER_DEV_NAME="eno1d1"

export MACE_PATH=`echo ${PWD%${PWD##*/}}`

#export NATIVE_PING_CMD="${MACE_PATH}/iputils/ping"
export NATIVE_PING_CMD="ping"
export CONTAINER_PING_CMD="/iputils/ping"

export PING_CONTAINER_IMAGE="chrismisa/slow-ping"
export PING_CONTAINER_NAME="ping-container"

export ANALYSIS_CMD="Rscript report_params.r"

export META_DATA="metadata"
export MANIFEST="manifest"


export PAUSE_CMD="sleep 5"

export DATE_STR=`date +%Y%m%d%H%M%S`

mkdir $DATE_STR
pushd $DATE_STR > /dev/null

echo -e "$B uname -a $B\n $(uname -a)\n" >> $META_DATA
echo -e "$B lshw: $B\n $(lshw)\n" >> $META_DATA

echo $B Running params test $B

#
# Spin up container
#
docker run -itd --name=$PING_CONTAINER_NAME \
	  --entrypoint=/bin/bash \
	  --cpuset-cpus=0-0 \
	  $PING_CONTAINER_IMAGE \
	  > /dev/null
echo "  Ping container up"

for R in `seq 1 $NUM_ROUNDS`
do
  echo $B Round $R $B
  for S in $SIZES
  do

    echo "  Running with size: $S"

    #
    # Native Control userspace base-line
    #
    taskset 0x1 $NATIVE_PING_CMD $PING_ARGS -s $S $TARGET >> ${R}native_${S}.ping
    echo "    Took native control"
    echo "${R}native_${S}.ping" >> $MANIFEST
    $PAUSE_CMD
    
    #
    # Container Control base-line
    #
    docker exec $PING_CONTAINER_NAME \
      $CONTAINER_PING_CMD $PING_ARGS -s $S $TARGET >> ${R}container_${S}.ping
    echo "    Took container control"
    echo "${R}container_${S}.ping" >> $MANIFEST
    $PAUSE_CMD

  done
done

popd > /dev/null

echo "  Generating graphs and analysis"
$ANALYSIS_CMD $DATE_STR

docker stop $PING_CONTAINER_NAME > /dev/null
docker rm $PING_CONTAINER_NAME > /dev/null

echo $DATE_STR is done.
