#!/bin/bash

for (( ; ; ))
do
	# Run script with arg from 10 to 200
	./pthread_mutex_condvar $((RANDOM%200+10))

	if [ $? -eq 0 ]; then
		sleep 3
	else
		break;
	fi
done
