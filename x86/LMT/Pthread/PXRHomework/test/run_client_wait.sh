#!/bin/bash

PORT=9999
ADDR='127.0.0.1'
MESSAGE="HELLO"

echo "Running client in mode waiting ..."
python ../TcpClient.py -p ${PORT} -a ${ADDR} -m ${MESSAGE} -w "yes"