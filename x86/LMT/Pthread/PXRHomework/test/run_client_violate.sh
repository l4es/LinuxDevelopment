#!/bin/bash

PORT=9999
ADDR='127.0.0.1'
MESSAGE="HELLO!"

echo "Sending ${MESSAGE}  ..."
python ../TcpClient.py -p ${PORT} -a ${ADDR} -m ${MESSAGE} -w "no"
sleep 1

MESSAGE="BONJOUR!"
while true; do
    echo ""
	echo "Sending ${MESSAGE}  ..."
	python ../TcpClient.py -p ${PORT} -a ${ADDR} -m ${MESSAGE} -w "no"
	sleep 3
done
