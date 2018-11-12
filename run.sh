#!/bin/bash

#
# Run preliminary mace module profiling
#
# Prerequisites:
#   mknod using mace mod's assigned Major number for the dev file
#

PWD=$(pwd)

MODULE_NAME="trace_test"
MODULE_PATH="${PWD}/mod/${MODULE_NAME}.ko"
DEV_FILE_PATH="${PWD}/mod/temp_out"
OUTER_DEV="3"



PING_PATH="${PWD}/iputils/ping"
TARGET_IPV4="10.10.1.2"
PING_ARGS="-i 1.0 -s 56"

PAUSE_CMD="sleep 10"

DATE_TAG=`date +%Y%m%d%H%M%S`
META_DATA="Metadata"

mkdir $DATE_TAG
cd $DATE_TAG

# Get some basic meta-data
echo "uname -a -> $(uname -a)" >> $META_DATA
echo "docker -v -> $(docker -v)" >> $META_DATA
echo "lsb_release -a -> $(lsb_release -a)" >> $META_DATA
echo "sudo lshw -> $(lshw)" >> $META_DATA
echo "/proc/cpuinfo -> $(cat /proc/cpuinfo)" >> $META_DATA


echo "*** Native Control ***"
$PING_PATH $PING_ARGS $TARGET_IPV4 > native_control_${TARGET_IPV4}.ping &
PING_PID=$!
echo "Pinging"

$PAUSE_CMD

kill -INT $PING_PID
echo "Killed ping"

echo "Done."

echo "*** Native Monitored ***"
$PING_PATH $PING_ARGS $TARGET_IPV4 > native_monitored_${TARGET_IPV4}.ping &
PING_PID=$!
echo "Pinging"

insmod $MODULE_PATH outer_dev=$OUTER_DEV inner_pid=$PING_PID
[ $? -eq 0 ] && echo "Module inserted" || echo "Failed to insert module"

cat $DEV_FILE_PATH > native_monitored_${TARGET_IPV4}.lat &
CAT_PID=$!
echo "cating latencies from $DEV_FILE_PATH"

$PAUSE_CMD

kill $CAT_PID
echo "Killed cat"

kill -INT $PING_PID
echo "Killed ping"

rmmod $MODULE_NAME
echo "Removed module"

echo "Done."
