#!/bin/bash

export ip
export delay
export beginningPort
export numContainers
export i
ip=$1
numberOfServers=$2
numberOfClients=$3
delay=$4
FILE=var.txt

if [ -f $FILE ];
then
        read beginningPort < $FILE
else
        beginningPort=8080
fi

endingPort=$(($beginningPort + $numberOfServers -1)) #defines the range of ports the server containers can use when scaling with docker-compose
BPORT=$beginningPort EPORT=$endingPort docker-compose up --scale nginx=$numberOfServers -d #create server containers and pass in beginningPort andendingPort as environment variables to compose file


copyNumServers=$numberOfServers # used to divide by number of remaining servers that still need containers to be distributed to them
copyNumClients=$numberOfClients #used to decrement by number of containers just assigned


for ((i=0; i < numberOfServers; i++));
do
	numContainers=$((copyNumClients / copyNumServers))
	bash -c 'ssh node1 IP=$ip PORT=$beginningPort DELAY=$delay COMPOSE_PROJECT_NAME=client$i$beginningPort docker-compose up -d --scale clientContainers=$numContainers ' &
	copyNumClients=$(( copyNumClients - numContainers ))
        (( copyNumServers-- ))
        (( beginningPort++ ))

done
wait

echo $beginningPort > $FILE
#for ((i=0; i < numberOfClients; i++));
#do	index=$(($i%$numberOfServers))
#	serverPort=${ports[$index]}
#	ssh node1 docker run -d clients $ip $serverPort $delay
#done


#for ((i=0; i < numberOfServers; i++));
#do
#       docker run -d --name server$port -p $port:80 servers
#       ports+=($port)
#       ((port++))
#done
