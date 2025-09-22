/**************************************************************
 * Chat Client
 * 
 * Description:
 * This program is a simple multithreaded chat server program
 * using pthreads. It allows two users to 
 * talk by sending and receiving messages in real time.
 *
 * It can get the username during startup, connect to the host,
 * exchange usernames with the host after connection, can support 
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
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

//Global Variiables for our and other user's usernames
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

//Thread function for recieve message from the other user
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

		//prints received message
        printf("%s\n", buffer);
		printf("<%s> ", username);
        fflush(stdout);
    }

    if (ret < 0)
        perror("Error receiving data");
    else {
        printf("\n Connection closed by remote\n");
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

		//check for exit
        if (checkExitMsg(buffer)) {
            write(sockfd, "exit", 4);
            printf("Exiting...\n");
            close(sockfd);
            exit(0);
        }

        char message[300];
        snprintf(message, sizeof(message), "\n<%s> %s", username, buffer);

		//send message to other user
        n = write(sockfd, message, strlen(message));
        if (n < 0)
            error("ERROR writing to socket");
		printf("<%s> ", username);
    }

    return NULL;
}

//start point of the program, sets up all parts of the program
int main(int argc, char *argv[]) {
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t rThread, sThread;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
        return 1;
    }

	//gets user's username
    printf("Provide user name: ");
    if (!fgets(username, sizeof(username), stdin))
        error("ERROR reading username");
    username[strcspn(username, "\n")] = 0;

	//socket setup
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, (size_t)server->h_length);
    serv_addr.sin_port = htons((unsigned short)portno);

	//Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    write(sockfd, username, strlen(username));
    read(sockfd, remoteUsername, sizeof(remoteUsername));

    printf("Connection established with %s (%s)\n", argv[1], remoteUsername);
	printf("<%s> ", username);

	//create threads for send/recieving messages
    if (pthread_create(&sThread, NULL, sendMessage, &sockfd))
        error("ERROR creating send thread");

    if (pthread_create(&rThread, NULL, receiveMessage, &sockfd))
        error("ERROR creating receive thread");

	//wait for threads to finish
    pthread_join(sThread, NULL);
    pthread_join(rThread, NULL);

	//clean up
    close(sockfd);
    return 0;
}