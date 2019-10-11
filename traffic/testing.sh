#!/bin/bash
FILE=var.txt
if [ -f $FILE ];
then
	read port < FILE
else
	port=8080
fi

echo $port
