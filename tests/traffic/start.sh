#!/bin/bash

export IP=$1
export DELAY=$4
export BEGINNING_PORT
export NUM_CONTAINERS
export I
export COMPOSE="$(pwd)/../traffic/docker-compose.yml"
numberOfServers=$2
numberOfClients=$3
FILE=var.txt

echo path from start: $COMPOSE
echo pwd: $(pwd)

#if previous port numbers have been used, read in most recently used. Else, default to 8080.
if [ -f $FILE ];
then
        read BEGINNING_PORT < $FILE
else
        BEGINNING_PORT=8080
fi

endingPort=$(($BEGINNING_PORT + $numberOfServers -1)) #defines the range of ports the server containers can use when scaling with docker-compose
BPORT=$BEGINNING_PORT EPORT=$endingPort COMPOSE_PROJECT_NAME=server$BEGINNING_PORT docker-compose -f $COMPOSE up --scale nginx=$numberOfServers -d #create server containers and pass in beginningPort andendingPort as environment variables to compose file


copyNumServers=$numberOfServers # used to divide by number of remaining servers that still need containers to be distributed to them
copyNumClients=$numberOfClients #used to decrement by number of containers just assigned

#create client containers and distribute even workload each server based on port number
for ((I=0; I < numberOfServers; I++));
do
	NUM_CONTAINERS=$((copyNumClients / copyNumServers))
	bash -c 'ssh node1 IP=$IP PORT=$BEGINNING_PORT DELAY=$DELAY COMPOSE_PROJECT_NAME=client$I$BEGINNING_PORT docker-compose up -d --scale clientContainers=$NUM_CONTAINERS ' &
	copyNumClients=$(( copyNumClients - NUM_CONTAINERS ))
        (( copyNumServers-- ))
        (( BEGINNING_PORT++ ))

done
wait


#write the last port used to the file var txt to be used again as the starting point for the next execution of this script
echo $BEGINNING_PORT > $FILE


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
