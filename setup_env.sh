#!/bin/bash

LINUX_VERSION=`uname -r | egrep -o '[0-9].*-[0-9]*[0-9]'`

apt-get update && apt-get install linux-headers-${LINUX_VERSION}
