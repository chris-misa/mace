#!/bin/bash
docker rm $(docker stop $(docker ps -a -q --filter ancestor=clients))
