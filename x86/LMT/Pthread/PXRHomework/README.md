1. How it works:
===============
- The main idea is creating a thread associating a client connection (socket number)  
- Each thread listens and reacts to behavior of associated client on the socket 

2. How to build: 
===============
$ cd /PXRHomework  
$ make

3. How to test:
===============
See /test/README.md  

4. Folder layout:  
================  
PXRHomework  
├── Makefile  
├── README.md  
├── src  
│   ├── client.cpp  
│   ├── client.h  
│   ├── customthread.cpp  
│   ├── customthread.h  
│   ├── main.cpp  
│   ├── server.cpp  
│   └── server.h  
├── TcpClient.py  
└── test  
    ├── run_client_loop.sh  
    ├── run_client_one_shot.sh  
    ├── run_client_parallel.sh  
    ├── run_client_violate.sh  
    ├── run_client_wait.sh  
    └── run_server.sh  
    
