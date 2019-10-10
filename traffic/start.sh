#!/bin/bash

numberOfServers=$1
numberOfClients=$2
delay=$3

ipAddresses=()
port=8080
for ((i=0; i < numberOfServers; i++));
do 
	serverName=server$i
	docker run -p $port:80 -d --name $serverName servers
	serverIp=$(docker inspect -f "{{ .NetworkSettings.IPAddress }}" $serverName)
	ipAddresses+=($serverIp)
	((port++))
done

# ssh node1


port=8080
for ((i=0; i < numberOfClients; i++));
do
	ip=$(($i%$numberOfServers))
	docker run -d client$i $ip $delay
	((port++))
done

