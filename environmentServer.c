#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include <X11/Xlib.h>
#include "simulator.h"





Environment    environment;  // The environment that contains all the robots




// Handle client requests coming in through the server socket.  This code should run
// indefinitiely.  It should repeatedly grab an incoming messages and process them. 
// The requests that may be handled are STOP, REGISTER, CHECK_COLLISION and STATUS_UPDATE.   
// Upon receiving a STOP request, the server should get ready to shut down but must
// first wait until all robot clients have been informed of the shutdown.   Then it 
// should exit gracefully.  

void *handleIncomingRequests(void *e) {

	//Type casting void pointer to environment pointer
	Environment *environment;
	environment = (Environment *) e;
	
	char   online = 1;
  	// ... ADD SOME VARIABLE HERE... //
  	
  	//Variables for the environmentServer
  	int serverSocket;
  	int clientSocket;
  	struct sockaddr_in serverAddr;
  	struct sockaddr_in clientAddr;
  	int status;
  	int addrSize;
  	int bytesReceived;
  	fd_set readfds;
  	fd_set writefds;
  	
  	unsigned char buffer[8];		//Array that holds information coming from the client
  	unsigned char clientCommand;		//Stores the information of buffer[0] which is the command the client is sending
  	unsigned char okResponse =OK;		//Equivalent to the OK response
  	unsigned char noResponse = NOT_OK;	//Equivalent to the NOT_OK response
  	unsigned char response[8];		//Array that holds the information being sent to the client
  	environment->numRobots =0;		//Setting numRobots to 0
  	srand(time(NULL));			//Seed for random numbers
  	unsigned char collision[8];		//Array to store information from the buffer, used in CHECK_COLLISION
  	unsigned char update[8];		//Array to store information from the buffer, used in STATUS_UPDATE
  	unsigned char kill=0;			//Flag used for shutting down the server and client
  	int count =0;				//Used to keep a count

  	// Initialize the server
  	
  	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  	
  	if(serverSocket<0){
  		printf("Server error: could not open socket. \n");
  		exit(-1);
  	}
  	
  	memset(&serverAddr, 0, sizeof(serverAddr));
  	serverAddr.sin_family = AF_INET;
  	serverAddr.sin_addr.s_addr =htonl(INADDR_ANY);
  	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);
  	
  	status =bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
  	
  	if(status<0){
  		printf("Server error: could not bind socket. \n");
  		exit(-1);
  	}

  	// Wait for clients now
  	
	while (online) {
		// ... WRITE YOUR CODE HERE ... //
		
		FD_ZERO(&readfds);
		FD_SET(serverSocket, &readfds);
		FD_ZERO(&writefds);
		FD_SET(serverSocket, &writefds);
		status = select(FD_SETSIZE, &readfds, &writefds, NULL, NULL);
		
		if(status ==0){//Timeout occured, no client ready
		}
		
		else if(status <0){
			printf("Server Error: Could not select socket. \n");
			exit(-1);
		}
		
		else{
			addrSize =sizeof(clientAddr);
			bytesReceived = recvfrom(serverSocket, buffer, 8,0,(struct sockaddr *) &clientAddr, &addrSize);
			
			if(bytesReceived >0){
				buffer[bytesReceived] ='\0';
				
			
				clientCommand = buffer[0];
				
				//If environmentServer receives STOP command:
				//It will set a flag to one if there are robots registered.
				//If there are no registered robots, it will break out of the while loop and close the socket.
				
				if(clientCommand == STOP){
					
					//If the server shuts down before any robots are registered
					if(count == environment->numRobots){
						break;
					}
				
					kill =1;
				}
			
				//If environmentServer receives REGISTER command, it will register the robots if numRobots is less than MAX_ROBOTS
				
				if(clientCommand ==REGISTER){
				
					unsigned char directionSign;	//Stores if the direction is positive or negative
					int angle;			//Stores only positive direction between 0 to 180
					unsigned char valid =0;		//flag to determine if unique location is found
					
					if(environment->numRobots <20){
					
						Robot r;	
						float x;
						float y;
						
						angle =(int) rand()%181;		//Random angle between 0 to 180
						directionSign = (int) rand()%2;		//Randomly choosing if direction should be positive or negative
						
						response[0] =okResponse;		//okResponse is equivalent to OK command being sent to the robotClient
						
						if(directionSign ==0){			
							response[2] =0;			//if directionSign is 0, the angle (direction) is positive
							r.direction = angle;
							
						}
						else{					
							response[2] =1;
							r.direction = angle*-1;		//if directionSign is 1, the angle (direction) is negative
							
							
						}
						
						//Ensures that each robot is placed in an unique non-overlapping location in the environment
						while(1){
						
							unsigned char location =0;
							int distance;
							
							//Generating random x and y 
							x = rand()% (ENV_SIZE-2*ROBOT_RADIUS)+ROBOT_RADIUS;
							y = rand()% (ENV_SIZE-2*ROBOT_RADIUS)+ROBOT_RADIUS;

							if(environment->numRobots ==0){
								r.x =x;
								r.y =y;
								valid =1;
								break;
							}
							else{
								for(int i =0; i<environment->numRobots; i++){
									distance = sqrt(pow((x - environment->robots[i].x),2) + pow((y - environment->robots[i].y),2));
									if(distance <= (2 *ROBOT_RADIUS)){
										location =1;	//invalid location
									}
								}
								if(location ==1){
									continue;
								}
								else{
									r.x =x;
									r.y =y;
									valid =1;
									break;
								}
							}
							
						}
						
						
						
						environment->robots[environment->numRobots]= r;
						
						//Multiplied by 10 to take 1 decimal place into account for calculation
						x = x*10;
						y=y*10;
						
						response[1] = environment->numRobots;	//ID of the Robot
						response[3]=  angle;			//Stores only the positive direction
						
						//Splitting x and y into high and low bytes
						
						response[4]=  (unsigned)x%256; 
						response[5]=  (unsigned)x/256; 
						response[6]=  (unsigned)y%256; 
						response[7]=  (unsigned)y/256;
						
						
						environment->numRobots++;
						
						sendto(serverSocket, response, 8, 0, 
						(struct sockaddr *) &clientAddr,addrSize);
									
					}
					else{
						
						//Sending NOT_OK if there are already 20 registered Robots
						
						response[0] =noResponse;	//Equivalent to NOT_OK command
						sendto(serverSocket, response, 1, 0, 
						(struct sockaddr *) &clientAddr,addrSize);
					}
					
				}
				
				//If environmentServer receives CHECK_COLLISION, the server would check if the robot would collide with a boundary or another robot and send response accordingly.
				
			
				if(clientCommand == CHECK_COLLISION){
				
					float newX;
					float newY;
					int flagCollide =0;
					int turn;
					int angle;
					int direction;
					int distance;
					
					
					collision[1]=buffer[1];	//ID of the Robot
					collision[2]=buffer[2];	//Stores 0 if direction is positive or 1 if direction is negative
					collision[3]=buffer[3];	//Stores positive direction only between 0 to 180
					
					//High and low bytes of x 
					collision[4]=buffer[4];	
					collision[5]=buffer[5];	
					
					//High and low bytes for y 
					collision[6]=buffer[6];	
					collision[7]=buffer[7];	
					
					
					//If the environmentServer receives a STOP command, it would send LOST_CONTACT command to the robotClient.
					//If LOST_CONTACT command has been sent to all the registered robots, the while loop will break and the environmentServer would also shut down
					
					if(kill ==1){
						collision[0] = LOST_CONTACT;
						sendto(serverSocket, collision, 1, 0, 
						(struct sockaddr *) &clientAddr,addrSize);
						count++;
						if(count == environment->numRobots){
							break;
						}
					}
				
					
					else{
					
						newX = ((float)collision[4])+((float)collision[5]*256);
						newY = ((float)collision[6])+((float)collision[7]*256);
						
						//To take the first decimal place into account
						
						newX=newX/10;
						newY =newY/10;
						
						//Getting the actual positive or nagative value of the direction
						
						if(buffer[2] ==1){
							direction = buffer[3]*-1;
							
						}
						else{
							direction = buffer[3];
						}
						
						//Calculation to determine if robot can move
						
						newX =newX + ROBOT_SPEED * cos(direction *PI/180); 
						newY = newY +ROBOT_SPEED*sin(direction* PI/180);
						
						
						//Check to see if the robot would collide with a wall(boundary)
					
						if((newX+ROBOT_RADIUS >= ENV_SIZE) || (newX - ROBOT_RADIUS <=0) || (newY+ROBOT_RADIUS >= ENV_SIZE) || (newY - ROBOT_RADIUS <=0)){
								collision[0] = NOT_OK_BOUNDARY;
								flagCollide =1;
								
						}
						
						//Check to see if the robot would collide with another robot
						
						for(int i =0; i <environment->numRobots; i++){
							if(collision[1]!= i){
								distance = sqrt(pow((newX - environment->robots[i].x),2) + pow((newY - environment->robots[i].y),2));
								if(distance <= (2 *ROBOT_RADIUS)){
									collision[0] = NOT_OK_COLLIDE;
									flagCollide =1;
								}
							}
							else{
								continue;
							}
						}
						
						
						
						//If a robot does not collide with the boundary and with another robot, then a OK response is sent.
						//Otherwise a NOT_OK_BOUNDARY or NOT_OK_COLLIDE response is sent
					
						if(flagCollide ==0){
							collision[0] = OK;
						
							
							
							
							sendto(serverSocket, collision, 8, 0, 
							(struct sockaddr *) &clientAddr,addrSize);
							
							
							
						}
						else{
						
							sendto(serverSocket, collision, 8, 0, 
							(struct sockaddr *) &clientAddr,addrSize);

						}

						
					
							
							
					}
					
		
					
					
				}
				
				//STATUS_UPDATE allows the server to know the latest location of the robot and allows to display it.
				//This is also used for collision detection
			
				if(clientCommand == STATUS_UPDATE){
					int direction;
					float newX;
					float newY;
					
					update[1]=buffer[1];	//ID of the robot
					update[2]=buffer[2];	//Stores 0 if direction is positive or 1 if direction is negative
					update[3]=buffer[3];	//Stores positive direction only between 0 to 180
					
					//High and low bytes of x
					update[4]=buffer[4];	
					update[5]=buffer[5];	
					
					//High and low bytes of y
					update[6]=buffer[6];	
					update[7]=buffer[7];	
					
					newX = ((float)update[4])+((float)update[5]*256);
					newY = ((float)update[6])+((float)update[7]*256);
					
					//To take the first decimal place into account
					newX=newX/10;
					newY =newY/10;
					
					//Getting the actual positive or nagative value of the direction
					if(update[2] ==1){
						direction = update[3]*-1;
						
					}
					else{
						direction = update[3];
					}
					
					
					//The environmentServer is informed of the latest location of the robot and allows to display it
					environment->robots[buffer[1]].direction = direction;
					environment->robots[buffer[1]].x = newX;
					environment->robots[buffer[1]].y = newY;
				}

		
			}
			
		}
	
  	}
  	//The socket will close and the thread exits.
  	environment->shutDown =1;
  	close(serverSocket);
  	close(clientSocket);
	
	pthread_exit(NULL);
  	
}




int main() {
	// So far, the environment is NOT shut down
	environment.shutDown = 0;
	
	
  
	// Set up the random seed
	srand(time(NULL));

	// Spawn an infinite loop to handle incoming requests and update the display
	pthread_t t1;
	pthread_t t2;
	
	pthread_create(&t1, NULL, handleIncomingRequests, (void *)(&environment));
	pthread_create(&t2, NULL, redraw, (void *)(&environment));

	

	// Wait for the update and draw threads to complete
	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	
}