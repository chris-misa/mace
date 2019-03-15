#!/bin/bash

#
# Execute the one-shot routine once for a series of added iperf traffic pairs
#

IPERF_PAIRS_MAX=32
IPERF_PAIRS_STEP=1

export TARGET_CPU=0 # Starting point to RR CPU assignment of iperf server / client pairs
MAX_CPUS=16 # Total number of CPUs to use on this machine

export B="===================="

export NUM_ROUNDS=5

export PING_ARGS="-D -i 0.0 -s 1472 -c 2000"
export TARGET="10.10.1.2"

export TRACE_CMD_ARGS="-e syscalls:sys_enter_sendto -e syscalls:sys_exit_recvmsg -e net:net_dev_start_xmit -e net:netif_receive_skb"

export OUTER_DEV_ID=3
export OUTER_DEV_NAME="eno1d1"

export MACE_PATH=`echo ${PWD%${PWD##*/}}`

export NATIVE_PING_HW_CMD="${MACE_PATH}/iputils_hw/ping"
export NATIVE_PING_CMD="${MACE_PATH}/iputils/ping"
export CONTAINER_PING_CMD="/iputils/ping"

export PING_CONTAINER_IMAGE="chrismisa/slow-ping"
export PING_CONTAINER_NAME="ping-container"

export EXEC_ONE_SHOT_CMD="$(pwd)/one_shot.sh"
export ADD_IPERF_PAIR_CMD="$(pwd)/add_container_ping_remote.sh"
export BG_PING_ARGS="-f 10.10.1.2"

export META_DATA="metadata"
export MANIFEST="manifest"


export PAUSE_CMD="sleep 5"

export DATE_STR=`date +%Y%m%d%H%M%S`

mkdir $DATE_STR
pushd $DATE_STR > /dev/null

echo -e "$B uname -a $B\n $(uname -a)\n" >> $META_DATA
echo -e "$B lshw: $B\n $(lshw)\n" >> $META_DATA

echo $B Running Ftrace Perturbation Comparison test $B

for iperf_pairs in `seq 0 $IPERF_PAIRS_STEP $IPERF_PAIRS_MAX`
do
	echo $B $iperf_pairs pairs $B

	[ $iperf_pairs -eq 0 ] || {
		for i in `seq 1 $IPERF_PAIRS_STEP`
		do
			$ADD_IPERF_PAIR_CMD
			TARGET_CPU=$(( (TARGET_CPU + 1) % MAX_CPUS ))
		done
		echo "  Added another $IPERF_PAIRS_STEP ping container" 
	}

	mkdir $iperf_pairs
	pushd $iperf_pairs > /dev/null

  for i in `seq 1 $NUM_ROUNDS`
  do

    echo $B Round $i $B

    #
    # Native Control userspace base-line
    #
    taskset 0x1 $NATIVE_PING_CMD $PING_ARGS $TARGET >> native_control.ping
    echo "  Took native control"
    $PAUSE_CMD

    #
    # Spin up container
    #
    docker run -itd --name=$PING_CONTAINER_NAME \
                    --entrypoint=/bin/bash \
                    --cpuset-cpus=0-0 \
                    $PING_CONTAINER_IMAGE \
                    > /dev/null
    echo "  Ping container up"

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
    ####################### Take mace perturbation ########################
    #
    echo $B Taking MACE Perturbation $B


    #
    # Insert module
    #
    insmod ${MACE_PATH}mace.ko outer_dev=$OUTER_DEV_ID
    [ $? -eq 0 ] || { echo "Failed to insert module"; exit; }
    echo "  Inserted module"

    echo 1 > /sys/class/mace/on
    echo "  Mace active in native net namespace"

    $PAUSE_CMD

    #
    # Native Monitored for perturbation
    #
    taskset 0x1 $NATIVE_PING_CMD $PING_ARGS $TARGET >> mace_native_monitored.ping
    echo "  Took native monitored"

    echo 0 > /sys/class/mace/on
    echo "  Mace detached in native net namespace"

    #
    # Restart container
    # 
    docker run -itd --name=$PING_CONTAINER_NAME \
                    --device /dev/mace:/dev/mace \
                    -v /sys/class/mace:/mace \
                    --entrypoint=/bin/bash \
                    --cpuset-cpus=0-0 \
                    $PING_CONTAINER_IMAGE \
                    > /dev/null
    echo "  Ping container up"

    docker exec $PING_CONTAINER_NAME \
      bash -c 'echo 1 > /mace/on'
    echo "  Mace active in ping container"

    $PAUSE_CMD

    #
    # Container monitored
    #
    echo 1 > /sys/class/mace/sync
    docker exec $PING_CONTAINER_NAME \
      $CONTAINER_PING_CMD $PING_ARGS $TARGET >> mace_container_monitored.ping
    echo "  Took container monitored"

    docker exec $PING_CONTAINER_NAME \
      bash -c 'echo 0 > /mace/on'
    echo "  Mace detached in ping container"

    $PAUSE_CMD
    docker stop $PING_CONTAINER_NAME > /dev/null
    docker rm $PING_CONTAINER_NAME > /dev/null

    #
    # Remove module
    #
    rmmod mace
    echo "  Mace module removed"

    $PAUSE_CMD

    #
    #################### Take ftrace perturbation ###############################
    #
    echo $B Taking Ftrace Perturbation $B


    #
    # Native ping with ftrace
    #
    trace-cmd record $TRACE_CMD_ARGS > /dev/null &
    TRACE_CMD_PID=$!
    $PAUSE_CMD
    echo "  trace-cmd recording on pid $TRACE_CMD_PID"

    $PAUSE_CMD
    taskset 0x1 $NATIVE_PING_CMD $PING_ARGS $TARGET >> ftrace_native_monitored.ping
    

    kill -INT $TRACE_CMD_PID > /dev/null
    $PAUSE_CMD
    rm trace.dat
    # trace-cmd report >> ftrace_native_report.trace
    echo "  Took ftraced native"


    #
    # Container ping with ftrace
    #
    docker run -itd --name=$PING_CONTAINER_NAME \
                    --entrypoint=/bin/bash \
                    --cpuset-cpus=0-0 \
                    $PING_CONTAINER_IMAGE \
                    > /dev/null
    echo "  Ping container up"

    trace-cmd record $TRACE_CMD_ARGS > /dev/null &
    TRACE_CMD_PID=$!
    $PAUSE_CMD
    echo "  trace-cmd recording on pid $TRACE_CMD_PID"


    $PAUSE_CMD
    docker exec $PING_CONTAINER_NAME \
      $CONTAINER_PING_CMD $PING_ARGS $TARGET >> ftrace_container_monitored.ping
    echo "  Took ftraced container "

    kill -INT $TRACE_CMD_PID > /dev/null
    $PAUSE_CMD
    rm trace.dat
    # trace-cmd report >> ftrace_container_report.trace
    echo "  Took ftraced native"

    $PAUSE_CMD
    docker stop $PING_CONTAINER_NAME > /dev/null
    docker rm $PING_CONTAINER_NAME > /dev/null
    echo " Stopped container"

  done

	popd > /dev/null

	echo $iperf_pairs >> $MANIFEST
done

popd > /dev/null

echo "  Shutting down containers"
docker stop `docker ps -aq` > /dev/null
docker rm `docker ps -aq` > /dev/null

echo $DATE_STR is done.
