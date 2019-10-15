#!/bin/bash

#ip address of node 0
ip=$1
#port number assigned to server container to make wget requests to
port=$2
#average target used for poisson distribution
average=$3

#create poisson distribution array from python script
delays=($(python3 poisson.py average | tr -d '[],'))

i=0
length="${#delays[@]}"
#go into continuous loop of making wget requests to server container defined by ip address and port number passed in. 
#sleep based on index given from poisson distribution array delays
while true
do
	wget $ip:$port
	i=$(($i%$length))
	sleep $i
	((i++))
done


