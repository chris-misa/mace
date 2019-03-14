#!/bin/bash

#
# Runs MACE against native and container latencies and report perturbation
#
# 2019, Chris Misa
#

export B="===================="

export NUM_ROUNDS=5

export PING_ARGS="-D -i 0.0 -s 1472 -c 10000"
export TARGET="10.10.1.2"

export OUTER_DEV_ID=3
export OUTER_DEV_NAME="eno1d1"

export MACE_PATH=`echo ${PWD%${PWD##*/}}`

export NATIVE_PING_CMD="${MACE_PATH}/iputils/ping"
export CONTAINER_PING_CMD="/iputils/ping"

export PING_CONTAINER_IMAGE="chrismisa/slow-ping"
export PING_CONTAINER_NAME="ping-container"

export META_DATA="metadata"
export MANIFEST="manifest"


export PAUSE_CMD="sleep 5"

export DATE_STR=`date +%Y%m%d%H%M%S`


mkdir $DATE_STR
pushd $DATE_STR > /dev/null

echo -e "$B uname -a $B\n $(uname -a)\n" >> $META_DATA
echo -e "$B lshw: $B\n $(lshw)\n" >> $META_DATA


#
# Insert module
#
insmod ${MACE_PATH}mace.ko outer_dev=$OUTER_DEV_ID
[ $? -eq 0 ] || { echo "Failed to insert module"; exit; }
echo "  Inserted module"

for i in `seq 1 $NUM_ROUNDS`
do

  echo $B Run $i $B
  echo $B Native Perturbation $B


  echo 1 > /sys/class/mace/on
  echo "  Mace active in native net namespace"

  $PAUSE_CMD

  echo 1 > /sys/class/mace/pert_sys_enter
  echo 1 > /sys/class/mace/pert_net_dev_start_xmit
  echo 1 > /sys/class/mace/pert_netif_receive_skb
  echo 1 > /sys/class/mace/pert_sys_exit
  echo "  Reset mace perturbation counters"

  #
  # Native Monitored for perturbation
  #
  taskset 0x1 $NATIVE_PING_CMD $PING_ARGS $TARGET >> native_monitored.ping
  echo "  Took native monitored"

  cat /sys/class/mace/pert_sys_enter >> native_sys_enter.pert
  cat /sys/class/mace/pert_net_dev_start_xmit >> native_net_dev_start_xmit.pert
  cat /sys/class/mace/pert_netif_receive_skb >> native_netif_receive_skb.pert
  cat /sys/class/mace/pert_sys_exit >> native_sys_exit.pert
  echo "  Retrieved native perturbations"

  echo 0 > /sys/class/mace/on
  echo "  Mace detached in native net namespace"


  echo $B Container Perturbation $B

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

  echo 1 > /sys/class/mace/pert_sys_enter
  echo 1 > /sys/class/mace/pert_net_dev_start_xmit
  echo 1 > /sys/class/mace/pert_netif_receive_skb
  echo 1 > /sys/class/mace/pert_sys_exit
  echo "  Reset mace perturbation counters"

  docker exec $PING_CONTAINER_NAME \
    $CONTAINER_PING_CMD $PING_ARGS $TARGET >> container_monitored.ping
  echo "  Took container monitored"

  docker exec $PING_CONTAINER_NAME \
    cat /sys/class/mace/pert_sys_enter >> container_sys_enter.pert
  docker exec $PING_CONTAINER_NAME \
    cat /sys/class/mace/pert_net_dev_start_xmit >> container_net_dev_start_xmit.pert
  docker exec $PING_CONTAINER_NAME \
    cat /sys/class/mace/pert_netif_receive_skb >> container_netif_receive_skb.pert
  docker exec $PING_CONTAINER_NAME \
    cat /sys/class/mace/pert_sys_exit >> container_sys_exit.pert
  echo "  Retrieved container perturbations"

  docker exec $PING_CONTAINER_NAME \
    bash -c 'echo 0 > /mace/on'
  echo "  Mace detached in ping container"


  $PAUSE_CMD
  docker stop $PING_CONTAINER_NAME > /dev/null
  docker rm $PING_CONTAINER_NAME > /dev/null


done

rmmod mace

echo Done.
