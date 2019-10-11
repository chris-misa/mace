#!/bin/bash

ip=$1
numberOfServers=$2
numberOfClients=$3
delay=$4

ports=()
FILE=var.txt
echo file is $FILE
if [ -f $FILE ];
then
        read port < $FILE
else
        port=8080
fi

for ((i=0; i < numberOfServers; i++));
do 
	docker run -d --name server$port -p $port:80 servers
	ports+=($port)
	((port++))
done
echo $port > $FILE

for ((i=0; i < numberOfClients; i++));
do
	index=$(($i%$numberOfServers))
	serverPort=${ports[$index]}
	ssh node1 docker run -d clients1 $ip $serverPort $delay
done

