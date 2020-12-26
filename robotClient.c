#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"




// This is the main function that simulates the "life" of the robot
// The code will exit whenever the robot fails to communicate with the server

int main() {
	// ... ADD SOME VARIABLE HERE ... //
	unsigned char command = REGISTER;		//Equivalent to REGISTER command
	unsigned char collision = CHECK_COLLISION;	//Equivalent to CHECK_COLLISION command
	unsigned char request[8];			//Array to store information that is being sent to the environmentServer
	
	//Variables for setting up the server
	int clientSocket;
	int addrSize;
	int bytesReceived;
	struct sockaddr_in serverAddr;
	
	unsigned char buffer[8];			//Array to store information that us being received from the environmentServer
	unsigned char received;				//Stores buffer[0] which is the command robotClient is receiving from the environmentServer
	
	unsigned char set =0;				//Determines whether or not to keep turning the same way

	
	

	// Set up the random seed
	srand(time(NULL));

	// Register with the server
  
	clientSocket =socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(clientSocket<0){
		printf("Client Error: could not open socket. \n");
		exit(-1);
	}

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr=inet_addr(SERVER_IP);
	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);

	// Send register command to server.  Get back response data
	// and store it.   If denied registration, then quit.
	
	addrSize =sizeof(serverAddr);
	
	sendto(clientSocket, &command, 1,0, (struct sockaddr *) &serverAddr, addrSize);
	
	bytesReceived = recvfrom(clientSocket, buffer, 8, 0, (struct sockaddr *) &serverAddr, &addrSize);
	
	received = buffer[0];
	
	//Close clientSOcket if there are already 20 registered robots
	if(received == NOT_OK){
		printf("Cannot add more than 20 robots, process quitting. \n");
		close(clientSocket);
		exit(0);
	}
	

	
	
	

	// Go into an infinite loop exhibiting the robot behavior
	
		
	request[0]= CHECK_COLLISION;
	request[1]=buffer[1];
	request[2]=buffer[2];
	request[3]=buffer[3];
	request[4]=buffer[4];
	request[5]=buffer[5];
	request[6]=buffer[6];
	request[7]=buffer[7];
	
	
	while (1) {
		
		// Check if can move forward
		request[0] = CHECK_COLLISION;
		sendto(clientSocket, request, 8,0, (struct sockaddr *) &serverAddr, addrSize);
		
		
		
	
		// Get response from server.
		
		bytesReceived = recvfrom(clientSocket, buffer, 8, 0, (struct sockaddr *) &serverAddr,&addrSize);
		if(bytesReceived>0){
			buffer[bytesReceived] ='\0';
		}
		
		
		// If ok, move forward
		if(buffer[0] == OK){
	
			
			int dir;
			request[0]= CHECK_COLLISION;	//Stores the CHECK_COLLISION command
			request[1]=buffer[1];	//ID of the robot
			request[2]=buffer[2];	//Stores 0 if direction is positive or 1 if direction is negative
			request[3]=buffer[3];	//Stores positive direction only between 0 to 180
			
			//Getting the actual positive or nagative value of the direction
			if(buffer[2] ==1){
				dir = buffer[3]*-1;
				
			}
			else{
				dir = buffer[3];
			}
			
		
			float newX;
			float newY;
			newX = ((float)buffer[4])+((float)buffer[5]*256);
			newY = ((float)buffer[6])+((float)buffer[7]*256);
			
			//Used to take the first decimal place into account
			newX=newX/10;
			newY =newY/10;
			
			//Calculation for the robot to move
			
			newX =newX + ROBOT_SPEED * cos(dir *PI/180); 
			newY = newY +ROBOT_SPEED*sin(dir* PI/180);
		
			
			//Used to take the first decimal place into account
			newX =newX*10;
			newY=newY*10;
	
			//Splitting x and y into high and low bytes
			request[4]=(unsigned)newX%256;
			request[5]=(unsigned)newX/256;
			request[6]=(unsigned)newY%256;
			request[7]=(unsigned)newY/256;
			
			
		}
		
		//If the robotClient receives LOST_CONTACT, the while loop would break and the socket will close.
		else if(buffer[0] == LOST_CONTACT){
			
			break;
		}
		
		// Otherwise, we could not move forward, so make a turn.
		// If we were turning from the last time we collided, keep
		// turning in the same direction, otherwise choose a random 
		// direction to start turning.
	
		else if (buffer[0] == NOT_OK_BOUNDARY||buffer[0] == NOT_OK_COLLIDE){ 
			
			
			int dir;
			int flag =1;
			unsigned char rotate;
			
			
			//If the robot could not move in any successive turns, it will continue to turn in the same direction.
			//Else, a random direction to turn towards (CW or CCW) is found
			if(set==0){
				int turn = (int) rand()%2;
				
				set =1;
				if(turn ==0){
				 	rotate=0;
				
				}
				else{
					rotate =1;
				}
			}
			
			//Getting the actual positive or nagative value of the direction
			if(buffer[2] ==1){
				dir = buffer[3]*-1;
				
			}
			else{
				dir = buffer[3];
			}
			
			//Turn CCW
			if(rotate ==0) {
				dir -=ROBOT_TURN_ANGLE; 
			}
			//Turn CW
			else if(rotate ==1) {
				dir +=ROBOT_TURN_ANGLE; 
			}
			
			//If direction is greater than 180, subtract 360 from the direction to bring the direction in the range
			if(dir >180){
				
				dir -= 360;
				request[2] =1;
			}
			//If direction is less than -180, add 360 to the direction to bring direction within the range
			else if(dir <-180){
				
				dir += 360;
				request[2] =0;
				
			}
			else{
				if(dir <0){
					request[2]=1;	//If direction is negative, store 1
				}
				else{
					request[2]=0;	//If direction is positive, store 0
				}
			}
			
			//Stores only positive direction
			
			if (request[2] ==1){
				request[3] = dir *-1; 
			}
			else{
				request[3] =dir;
			}
	
				
						
		}
		
	

		// Send update to server
		
		request[0] = STATUS_UPDATE;
		sendto(clientSocket, request, 8,0, (struct sockaddr *) &serverAddr, addrSize);
		
		
		usleep(20000);
	}
		
 
	//Close client socket when contact is lost
	close(clientSocket);
	return 0;
}
