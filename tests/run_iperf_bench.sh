#!/bin/bash

#
# Execute the one-shot routine once for a series of added iperf traffic pairs
#

IPERF_PAIRS_MAX=32
IPERF_PAIRS_STEP=1

export TARGET_CPU=0 # Starting point to RR CPU assignment of iperf server / client pairs
MAX_CPUS=16 # Total number of CPUs to use on this machine

export B="===================="

# Changing this will break correlation
export NUM_ROUNDS=1

export PING_ARGS="-D -i 0.0 -s 1472 -c 3000"

export TARGET="10.10.1.2"

export OUTER_DEV_ID=3
export OUTER_DEV_NAME="eno1d1"

export MACE_PATH=`echo ${PWD%${PWD##*/}}`

export NATIVE_PING_HW_CMD="${MACE_PATH}/iputils_hw/ping"
export NATIVE_PING_CMD="${MACE_PATH}/iputils/ping"
export CONTAINER_PING_CMD="/iputils/ping"

export PING_CONTAINER_IMAGE="chrismisa/slow-ping"
export PING_CONTAINER_NAME="ping-container"

export EXEC_ONE_SHOT_CMD="$(pwd)/one_shot.sh"
export ADD_IPERF_PAIR_CMD="$(pwd)/add_container_iperf_pair.sh"
#
# Note: since report_one_shot.r also has locality (pwd) dependencies this is a little tricky
# Probably need to wait till all data is gathered, then recursively go through manifest to run analysis
#
export ANALYSIS_CMD="Rscript report_one_shot.r"
export FINAL_ANALYSIS_CMD="Rscript report_iperf_bench.r"

export META_DATA="metadata"
export MANIFEST="manifest"


export PAUSE_CMD="sleep 5"

export DATE_STR=`date +%Y%m%d%H%M%S`


mkdir $DATE_STR
pushd $DATE_STR > /dev/null

echo -e "$B uname -a $B\n $(uname -a)\n" >> $META_DATA
echo -e "$B lshw: $B\n $(lshw)\n" >> $META_DATA

echo $B Running iperf traffic pairs test $B

for iperf_pairs in `seq 0 $IPERF_PAIRS_STEP $IPERF_PAIRS_MAX`
do
	echo $B $iperf_pairs pairs $B

	[ $iperf_pairs -eq 0 ] || {
		for i in `seq 1 $IPERF_PAIRS_STEP`
		do
			$ADD_IPERF_PAIR_CMD
			TARGET_CPU=$(( (TARGET_CPU + 1) % MAX_CPUS ))
		done
		echo "  Added another $IPERF_PAIRS_STEP iperf pairs" 
	}

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

echo "  Starting initial analysis"
mkdir ${DATE_STR}/cdfs
for i in `cat ${DATE_STR}/${MANIFEST}`
do
	$ANALYSIS_CMD ${DATE_STR}/${i}/
	mv ${DATE_STR}/${i}/cdfs.pdf ${DATE_STR}/cdfs/${i}_cdfs.pdf
done

$FINAL_ANALYSIS_CMD ${DATE_STR}/

echo $DATE_STR is done.
