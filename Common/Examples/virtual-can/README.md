 To install and test the virtual driver as following steps:

    modprobe can
    modprobe can-dev
    modprobe can-raw
    insmod vcan.ko
    ifconfig can0 up
    candump can0
    cansend can0 123#112233 
