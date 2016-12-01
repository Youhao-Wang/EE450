#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>


#define MYPORT "21859"   //my port number for ServerA
#define HOST "localhost"


int main(void){
	// set up UDP  -- From Beej
	int sockfd;
	int rv;
	struct addrinfo hints;  // the struct addrinfo have already been filled with relevant info
	struct addrinfo *servinfo; //point out the result
	struct sockaddr_storage their_addr;
	struct addrinfo *p;  //tempoary point
	socklen_t addr_len;


	memset(&hints, 0, sizeof hints);  // make sure the struct is empty
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // UDP dgram sockets
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(HOST, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 0;
	}
// loop through all the results and bind to the first we can----Beej
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("serverA: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("serverA: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "serverA: failed to bind socket\n");
		return 0;
	}
	freeaddrinfo(servinfo);
	printf( "The Server A is up and running using UDP on port %s.\n", MYPORT);


	while(1){
		addr_len = sizeof their_addr;
		char function[3];
		int pre_num = 0;
		int result = 0;

		recvfrom(sockfd, function, sizeof function , 0,(struct sockaddr *)&their_addr, &addr_len);
		recvfrom(sockfd, (char *)& pre_num, sizeof pre_num , 0,(struct sockaddr *)&their_addr, &addr_len);
		printf("The Server A has received %d numbers. \n", pre_num);
		int dataX[pre_num];
		//recvfrom(sockfd, (char *)& dataX, sizeof dataX, 0,(struct sockaddr *)&their_addr, &addr_len);

		recvfrom(sockfd, (const char *)&dataX, sizeof dataX, 0,(struct sockaddr *)&their_addr, &addr_len);
		//printf("temp = %d \n", temp);

		char * function_name = function;
		//make the calcuration
		if(strcmp(function_name, "SUM") == 0){
			int i;
			for(i = 0; i < pre_num; i++){
				result = result + dataX[i];	
			}
		}
		else if(strcmp(function_name,"SOS") == 0){
			int i;
			for(i = 0; i < pre_num; i++){
				int m = dataX[i];
				result = result + m * m;
			}
		}
		else if(strcmp(function_name,"MAX") == 0){
			int i;
			for(i = 0; i < pre_num; i++){
				if(dataX[i] > result )
					result = dataX[i];
			}
		}
		else if(strcmp(function_name, "MIN") == 0){
			int i;
			result = 2147483647;
			for(i = 0; i < pre_num; i++){
				if(dataX[i] < result )
					result = dataX[i];
			}
		}
		printf("The Server A has successfully finished the reduction %s : %d \n", function_name,result);
		
		//send back to aws
		sendto(sockfd, (char *)& result, sizeof result , 0,(struct sockaddr *) &their_addr, addr_len);
		printf("The Server A has successfully finished sending the reduction value to AWS server. \n");

	}
}