#!/bin/bash

#
# Execute the one-shot routine once for a series of added iperf traffic pairs
#

MAX_IPERF_PAIRS=3

export B="===================="

export NUM_ROUNDS=1

export PING_ARGS="-D -i 0.0 -s 1472 -c 2000"

export TARGET="10.10.1.2"

export OUTER_DEV_ID=5

export NATIVE_PING_CMD="ping"
export CONTAINER_PING_CMD="/iputils/ping"

export PING_CONTAINER_IMAGE="chrismisa/slow-ping"
export PING_CONTAINER_NAME="ping-container"

export EXEC_ONE_SHOT_CMD="$(pwd)/one_shot.sh"
export ADD_IPERF_PAIR_CMD="$(pwd)/add_container_iperf_pair.sh"
# export ANALYSIS_CMD="Rscript report_one_shot.r"

export META_DATA="metadata"
export MANIFEST="manifest"

export MACE_PATH=`echo ${PWD%${PWD##*/}}`

export PAUSE_CMD="sleep 5"

export DATE_STR=`date +%Y%m%d%H%M%S`

mkdir $DATE_STR
pushd $DATE_STR > /dev/null

echo -e "$B uname -a $B\n $(uname -a)\n" >> $META_DATA
echo -e "$B lshw: $B\n $(lshw)\n" >> $META_DATA

echo $B Running iperf traffic pairs test $B

for iperf_pairs in `seq 0 $MAX_IPERF_PAIRS`
do
	echo $B $iperf_pairs pairs $B

	[ $iperf_pairs -eq 0 ]
		|| ($ADD_IPERF_PAIR_CMD && echo "  Added another iperf pair")

	mkdir $iperf_pairs
	pushd $iperf_pairs > /dev/null

	$EXEC_ONE_SHOT_CMD

	popd > /dev/null

	echo $iperf_pairs >> $MANIFEST
done

popd > /dev/null

echo "  Shutting down containers"
docker stop `docker ps -aq` > /dev/null
docker rm `docker ps -aq` > /dev/null

echo Done.
