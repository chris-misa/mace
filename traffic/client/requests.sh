#!/bin/bash

ip=$1
port=$2
delay=$3

while true
do
	wget $ip:$port
	sleep $delay
done


