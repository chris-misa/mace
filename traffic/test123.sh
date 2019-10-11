#!/bin/bash
if [1]
then
	port=8080
else
	port=8085
fi
for ((i=0; i < 2; i++));
do 
	((port++))
done
echo port is now $port
