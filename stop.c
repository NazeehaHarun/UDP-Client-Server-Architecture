#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"


int main() {
  // ... ADD SOME VARIABLES HERE ... //
  unsigned char command = STOP;  //Equivalent to the STOP command
  
  //Variables for the server
  int clientSocket;
  int addrSize;
  int bytesReceived;
  struct sockaddr_in serverAddr;
  
  unsigned char buffer[1];	//Array to send the STOP command
  
  
  
  // Register with the server
  // ... WRITE SOME CODE HERE ... //
  clientSocket =socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(clientSocket<0){
  	printf("CLient Error: could not open socket. \n");
  	exit(-1);
  }
  
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr=inet_addr(SERVER_IP);
  serverAddr.sin_port = htons((unsigned short) SERVER_PORT);
  
  
  
  // Send command string to server
  // ... WRITE SOME CODE HERE ... //
  
  addrSize =sizeof(serverAddr);
  
  sendto(clientSocket, &command, 1,0, (struct sockaddr *) &serverAddr, addrSize);

  
  close(clientSocket);
  
  
  

	
}