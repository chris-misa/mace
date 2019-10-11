#!/bin/bash

numberOfServers=$1
numberOfClients=$2
delay=$3

ports=()
port=8080
ports+=($port)
for ((i=0; i < numberOfServers; i++));
do 
	serverName=server$i
	docker run -d --name $serverName servers
	ports+=($serverIp)
	((port++))
done

for ((i=0; i < numberOfClients; i++));
do
	index=$(($i%$numberOfServers))
	port=${ports[$index]}
	ssh node1 docker run -d --name client$port clients $port $delay
	((port++))
done

