STEPS TO TEST

1. Move to the test folder
$ cd test

2.a. Test a client who connects, sends "HELLO" and disconnects right after
Console 1: $./run_server.sh
==========

Console 2: $ ./run_client_one_shot.sh
=========

OUTPUT:

Console 1:
=========
Running server ...
Running!
Waiting client 0 ...
<--:HELLO
-->: HI
2 bytes sent.
Waiting client 0 ...
Client 0 diconnected
==>:BYEBYE

Console 2:
==========
-->: HELLO
<--: HI

2.b. Test a client who connects, sends "HELLO" and still listens on the socket while the other connects, sends "HELLO" and disconnects every 3s.
Console 1: $./run_server.sh
==========

Console 2: $./run_client_wait.sh
==========

Console 3: $./run_client_loop.sh
==========

OUTPUT:

Console 1: 
==========
Running server ...
Running!
Waiting client 0 ...
<--:HELLO
-->: HI
2 bytes sent.
Waiting client 0 ...
Waiting client 1 ...
<--:HELLO
-->: HI
2 bytes sent.
Waiting client 1 ...
Client 1 diconnected
 ==>:BYEBYE
Sending to 0
6 bytes sent.


Console 2: 
==========
Running client in mode waiting ...
-->: HELLO
<--: HI
<--: BYEBYE
<--: BYEBYE


Console 3: 
==========
Running client in mode periodic ...

-->: HELLO
<--: HI

-->: HELLO
<--: HI


2.c. Test a client who connects, sends "HELLO" and after that sends another message
Console 1: $./run_server.sh
==========

Console 2: $./run_client_violate.sh
==========

OUTPUT:

Console 1: 
==========
Running server ...
Running!
Waiting client 0 ...
<--:HELLO
-->: HI
2 bytes sent.
Waiting client 0 ...
Client 0 diconnected
 ==>:BYEBYE
Waiting client 0 ...
<--:HALOO
Client 0 violates the rule, close it

Console 2: 
==========
Sending HELLO  ...
-->: HELLO
<--: HI

Sending HALOO  ...
-->: HALOO
<--: 

Sending HALOO  ...
-->: HALOO
<--: 

2.d. Test multiple connections in parallel
Console 1: $./run_server.sh
==========

Console 2: $./run_client_parallel.sh
==========
