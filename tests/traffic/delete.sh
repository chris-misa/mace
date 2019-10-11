#!/bin/sh
ssh node1 < clients_delete.sh
docker rm $(docker stop $(docker ps -a -q --filter ancestor=servers))
