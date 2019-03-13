#!/bin/bash

#
# Execute the one-shot routine in the current traffic environment
#

# Use hwstamp_ctl to request hardware time stamps on outer interface

export B="===================="

export NUM_ROUNDS=3

export PING_ARGS="-D -i 0.0 -s 1472 -c 2000"

export TARGET="10.10.1.2"

export OUTER_DEV_ID=3
export OUTER_DEV_NAME="eno1d1"

export MACE_PATH=`echo ${PWD%${PWD##*/}}`

export NATIVE_PING_CMD="${MACE_PATH}/iputils/ping"
export NATIVE_PING_HW_CMD="${MACE_PATH}/iputils_hw/ping"
export CONTAINER_PING_CMD="/iputils/ping"

export PING_CONTAINER_IMAGE="chrismisa/slow-ping"
export PING_CONTAINER_NAME="ping-container"

export EXEC_ONE_SHOT_CMD="$(pwd)/one_shot.sh"
export ANALYSIS_CMD="Rscript report_one_shot.r"

export META_DATA="metadata"
export MANIFEST="manifest"


export PAUSE_CMD="sleep 5"

export DATE_STR=`date +%Y%m%d%H%M%S`

mkdir $DATE_STR
pushd $DATE_STR > /dev/null

echo -e "$B uname -a $B\n $(uname -a)\n" >> $META_DATA
echo -e "$B lshw: $B\n $(lshw)\n" >> $META_DATA

echo $B Running mace one-shot test $B

$EXEC_ONE_SHOT_CMD

popd > /dev/null

echo "  Generating graphs and analysis"
$ANALYSIS_CMD $DATE_STR

echo $DATE_STR is done.
