//-------------------------------------------------------------------------------------------------------//
Project #2
CSC/ECE 573
Internet Protocols 
//-------------------------------------------------------------------------------------------------------//
Submitted by:
	Saurabh Deswal (200065557)
	Siddarth Singh (200063693)
//-------------------------------------------------------------------------------------------------------//

//------------------------------------------Steps to execute---------------------------------------------//
1. Extract proj2.zip. It will contain 2 folders:
	a. gbnServer
	b. gbnClient

2. Navigate to gbnServer using terminal and compile the code:
	gcc gbnServer.c -o server
2.1. Execute server using:
	 ./server <portNo> <inputFile> <p>
	 ./server 7735 inputFile.txt 0.05

3. Open gbnClient on the same machine in another terminal or another machine and compile the code:
	gcc gbnClient.c -o client
3.1. Execute client using
	./client <ipAddr> <portNo> <transferFile> <N> <MSS>
	./client 127.0.0.1 7735 gcc_trace.txt 64 500 /* if using one machine and two terminals */
	./client 157.2.99.7 7735 gcc_trace.txt 64 500 /* if server is on another machine*/
4. After the transfer is complete, gcc_trace.txt will copied to inputFile.txt in gbnServer folder.
//-------------------------------------------------------------------------------------------------------//


