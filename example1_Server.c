/**************************************************************
 * Chat Server
 * 
 * Description:
 * This program is a simple multithreaded chat server program
 * using pthreads. It allows two users to 
 * talk by sending and receiving messages in real time.
 *
 * It can get the username during startup, wait for the client,
 * exchange usernames with the client after connection, can support 
 * for message send/receive with doing multiple in a row, and 
 * can exit gracefully if someone types "exit"
 *
 * Author: Wyatt Ronn and Dylan Christopherson
 * Date: 9/23/25
 **************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

//Global Variables for our and the other usernames
char username[32];
char remoteUsername[32];

//Prints error message and exits if needed
void error(const char *msg) {
    perror(msg);
    exit(1);
}

//Returns true if the message passed as an input is exit
int checkExitMsg(char *msg) {
    return strcmp(msg, "exit") == 0;
}

// Thread function for recieve message from the other user
void *receiveMessage(void *socket) {
    int sockfd = *(int *)socket;
    char buffer[256];
    int ret;

    while ((ret = read(sockfd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[ret] = '\0';

		//check for exit
        if (checkExitMsg(buffer)) {
            printf("\nRemote user exited.\n");
			printf("<%s> ", username);
            close(sockfd);
            exit(0);
        }

		//Display the message displayed
        printf("%s\n", buffer);
		printf("<%s> ", username);
        fflush(stdout);
    }

    if (ret < 0)
        perror("Error receiving data");
    else {
        printf("Connection closed by remote\n");
		printf("<%s> ", username);
	}

    close(sockfd);
    exit(0);
    return NULL;
}

//sends message to the other user using threads
void *sendMessage(void *socket) {
    int sockfd = *(int *)socket;
    char buffer[256];
    ssize_t n;

    while (1) {
        if (!fgets(buffer, sizeof(buffer), stdin))
            error("ERROR reading stdin");

        buffer[strcspn(buffer, "\n")] = '\0';

		//check if the user want to exit
        if (checkExitMsg(buffer)) {
            write(sockfd, "exit", 4);
            printf("Exiting...\n");
            close(sockfd);
            exit(0);
        }

        char message[300];
        snprintf(message, sizeof(message), "\n<%s> %s", username, buffer);

		//Send message to other user
        n = write(sockfd, message, strlen(message));
        if (n < 0)
            error("ERROR writing to socket");
		printf("<%s> ", username);
    }

    return NULL;
}

//start point of the program, sets up all parts of the program
int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t rThread, sThread;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }
	
	//get username from user
    printf("Provide user name: ");
    if (!fgets(username, sizeof(username), stdin))
        error("ERROR reading username");
    username[strcspn(username, "\n")] = 0;
	
	printf("Waiting for Connection...\n");

	//create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

	//Setup server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

	//bind socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

	//Listen for other user
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

	//Exchange username
    read(newsockfd, remoteUsername, sizeof(remoteUsername));
    write(newsockfd, username, strlen(username));

    printf("Connection established with %s (%s)\n",
           inet_ntoa(cli_addr.sin_addr), remoteUsername);

    printf("<%s> ", username);
	
	//Create both threads for send/recieving messages
    if (pthread_create(&sThread, NULL, sendMessage, &newsockfd))
        error("ERROR creating send thread");

    if (pthread_create(&rThread, NULL, receiveMessage, &newsockfd))
        error("ERROR creating receive thread");

	//Wait for threads to finish
    pthread_join(sThread, NULL);
    pthread_join(rThread, NULL);

	//Cleanup everything
    close(newsockfd);
    close(sockfd);
    return 0;
}