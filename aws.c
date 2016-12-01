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
#include <signal.h>

#define TCPPORT "25859"   //TCP port
#define UDPPORT "24859"		//UDP port
#define HOST "localhost"
#define BACKLOG 10 // how many pending connections queue will hold
#define PORTA "21859"
#define PORTB "22859"
#define PORTC "23859"


void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// using UDP to calculate the result for back-server
int calcu(char function_name[], int dataX[], int pre_num, char ch){
	int mysock;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char* backserver_port;

    if(ch == 'A')
    	backserver_port = PORTA;
    else if (ch == 'B')
    	backserver_port = PORTB;
    else if( ch == 'C')
    	backserver_port = PORTC;

    //printf("port number: %s \n", backserver_port);

    //set up UDP -- from Beej
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;


    if ((rv = getaddrinfo(HOST, backserver_port, &hints, &servinfo))
			!= 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and make a socket----Beej
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((mysock = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "talker: failed to bind socket\n");
		return 2;
	}


	//using UDP to send the data
	sendto(mysock, function_name, sizeof function_name, 0, p->ai_addr,p->ai_addrlen);
	sendto(mysock, (char *)& pre_num, sizeof pre_num, 0, p->ai_addr,p->ai_addrlen);
	int data_sent[pre_num];
	int i;
	for(i = 0; i < pre_num; i++)
		data_sent[i] = dataX[i];
	sendto(mysock,(const char *)& data_sent, sizeof data_sent, 0, p->ai_addr,p->ai_addrlen);
	printf("The AWS sent %d numbers to BackendServer %c. \n", pre_num, ch);

	int result = 0;
	recvfrom(mysock, (char *)& result, sizeof result, 0 , NULL, NULL);
	//printf("the result is %d \n", result);
	return result; 
}


int main(){
	//set up TCP --from Beej
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(HOST, TCPPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}
	freeaddrinfo(servinfo); // all done with this structure

	//listen
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	
	printf( "The AWS is up and running. \n");

	//the whole loop
	while(1){
		sin_size = sizeof their_addr;	
		new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");			
			exit(1);
		}


		// get the port of client
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
		struct sockaddr_in addrTheir;
		memset(&addrTheir, 0, sizeof(addrTheir));
		int len = sizeof(addrTheir);
		getpeername(new_fd, (struct sockaddr *) &addrTheir, (socklen_t *) &len);
		int client_port = addrTheir.sin_port;


		//receive all the inforamtion from client
		char function_name[3];
		int total_num;

		recv(new_fd, function_name, sizeof function_name, 0);	
		recv(new_fd, (char *)&total_num, sizeof total_num, 0);
		int data[total_num];
		recv(new_fd, (char *)&data, sizeof data, 0 );
		//int temp = recv(new_fd, (char *)&data, sizeof data, 0 );
		//printf("length : %d \n", temp);
		printf("The AWS has received %d numbers from the client using TCP over port %d. \n", total_num, client_port);

		//divide into three array
		int pre_num = total_num/3;
		int dataA[pre_num];
		int dataB[pre_num];
		int dataC[pre_num];
		int i, k;
		k = 0; 
		for( i = 0; i < pre_num; i++){
			dataA[i] = data[k];
			k++;
		}
		for( i = 0; i < pre_num; i++){
			dataB[i] = data[k];
			k++;
		}
		for( i = 0; i < pre_num; i++){
			dataC[i] = data[k];
			k++;
		}

		int resultA = calcu (function_name, dataA, pre_num, 'A');
		int resultB = calcu(function_name, dataB, pre_num, 'B');
		int resultC = calcu(function_name, dataC, pre_num, 'C');

		printf("The AWS received reduction result of %s from Backend-Server A using UDP over port %s and it is %d \n",
		function_name, PORTA, resultA);
		printf("The AWS received reduction result of %s from Backend-Server B using UDP over port %s and it is %d \n",
		function_name, PORTB, resultB);
		printf("The AWS received reduction result of %s from Backend-Server C using UDP over port %s and it is %d \n",
		function_name, PORTC, resultC);


		//calculate the final result
		char *function = function_name;
		int result = 0;
		if(strcmp(function, "SUM") == 0){
			result = resultA + resultB + resultC;
		}
		else if(strcmp(function,"SOS") == 0){
			result = resultA + resultB + resultC;
		}
		else if(strcmp(function,"MAX") == 0){
			result = resultA;
			if(resultB > result)
				result = resultB;
			if(resultC > result)
				result = resultC;
		}
		else if(strcmp(function, "MIN") == 0){
			result = resultA;
			if(resultB < result)
				result = resultB;
			if(resultC < result)
				result = resultC;
		}
		printf("The AWS has successfully finished the reduction %s: %d \n" ,function, result);

		send(new_fd, (const char *)&result, sizeof(result), 0);
		printf("The AWS has successfully finished sending the reduction value to client.\n");
		close(new_fd); 

	}
	
}