#!/bin/sh
docker rm $(docker stop $(docker ps -a -q --filter ancestor=servers))
ssh node1 docker kill $(docker ps -a -q --filter ancestor=clients)
ssh node1 docker rm $(docker ps -a -q --filter ancestor=clients)
