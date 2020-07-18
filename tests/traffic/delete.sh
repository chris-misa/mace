#!/bin/sh
export PATH_TO_CLIENTS_DELETE=$(pwd)/traffic/clients_delete.sh
#ssh node1 < clients_delete.sh
ssh node1 < $PATH_TO_CLIENTS_DELETE
docker rm $(docker stop $(docker ps -a -q --filter ancestor=servers))
