#!/bin/bash

MAX_COUNT=250

PORT=9999
ADDR='127.0.0.1'
MESSAGE="HELLO"

CMD="python ../TcpClient.py -p ${PORT} -a ${ADDR} -m ${MESSAGE} -w no"

counter=$MAX_COUNT
while [ $counter -gt 0 ]
do
  echo "Process \"$CMD\" started";
  $CMD & pid=$!
  PID_LIST+=" $pid";
  counter=$(( $counter - 1 ))
done

trap "kill $PID_LIST" SIGINT

echo "Parallel processes have started";

wait $PID_LIST

echo
echo "All processes have completed";
