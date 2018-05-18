import socket
import sys, getopt

myopts, args = getopt.getopt(sys.argv[1:],"p:a:m:w:")

for opt, arg in myopts:
    if opt == '-p':
        server_port=arg
    elif opt == '-a':
        host_ip=arg
    elif opt == '-m':
        message=arg
    elif opt == '-w':
        wait=arg
    else:
        print("Usage: %s -p port -a address -m message -w wait" % sys.argv[0])

# Initialize a TCP client socket using SOCK_STREAM
tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    # Establish connection to TCP server and exchange data
    tcp_client.connect((host_ip, int(server_port)))
    tcp_client.sendall(message.encode())
    print ("-->: {}".format(message))

    # Read data from the TCP server and close the connection
    while True :
        response = tcp_client.recv(1024)
        print ("<--: {}".format(response.decode()))
        if wait != 'yes' or not response :
            break

finally:
    tcp_client.close()