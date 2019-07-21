#!/bin/bash

#
# Execute the one-shot routine once for a series of added traffic pairs
#
# Note that some of the naming in this file is based on previous experiments
# which used iperf for traffic generation.
#
# The value of `ADD_IPERF_PAIR_CMD` (which is set to `add_container_ping_remote.sh` below)
# determines the actual traffic generation and can be changed as desired.
#

IPERF_PAIRS_STEP=10
IPERF_PAIRS_MAX=100

export TARGET_CPU=0 # Starting point for CPU assignment of traffic generating containers
MAX_CPUS=16         # Total number of CPUs to use on this machine

export B="===================="

export NUM_ROUNDS=5

export PING_ARGS="-D -i 0.0 -s 1472 -c 2000" # Arguments to hand to probe container's ping
export TARGET="10.10.1.2"                    # IP of target to probe

export OUTER_DEV_ID=3                        # Interface id of device connecting to target
export OUTER_DEV_NAME="eno1d1"               # Interface name of device connecting to target

export MACE_PATH=`echo ${PWD%${PWD##*/}}`    # Grab absolute path of `../` where mace module should have been built

export NATIVE_PING_HW_CMD="${MACE_PATH}/iputils_hw/ping" # Command to gather hardware RTT
export NATIVE_PING_CMD="${MACE_PATH}/iputils/ping"       # Command to gather native RTT
export CONTAINER_PING_CMD="/iputils/ping"                # Command (in probe container) to gather container RTT

export PING_CONTAINER_IMAGE="chrismisa/slow-ping"        # Probe container image (on dockerhub)
export PING_CONTAINER_NAME="ping-container"              # Name for probe container

export EXEC_ONE_SHOT_CMD="$(pwd)/one_shot.sh"                    # Script which executes all RTT and latency measurements for a single traffic setting
export ADD_IPERF_PAIR_CMD="$(pwd)/add_container_ping_remote.sh"  # Script to add another traffic continer
export BG_PING_ARGS="-f 10.10.1.2"                               # Arguments to hand to ping in traffic containers

export ANALYSIS_CMD="Rscript report_one_shot.r"              # Script to process results of a single traffic setting run
export FINAL_ANALYSIS_CMD="Rscript report_iperf_bench.r"     # Script to aggregate results from multiple traffic runs

export META_DATA="metadata" # Name of metadata file generated with system description
export MANIFEST="manifest"  # Name of manifest file with list of generated raw results (used by post-processing scripts)

export PAUSE_CMD="sleep 5"   # How long to pause between taking measurements

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
