#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void *receiveMessage(void *socket)
{
    int sockfd, ret;
    char buffer[256];
    sockfd = (int)socket;

    memset(buffer, 0, sizeof(buffer));
    while ((ret = read(sockfd, buffer, sizeof(buffer))) > 0)
    {
        printf("<CLIENT>: %s", buffer);
    }
    if (ret < 0)
        printf("Error receiving data!\n");
    else
        printf("Closing connection\n");
    close(sockfd);
}

void *sendMessage(void *socket)
{
    int sockfd;
    char buffer[256];
    sockfd = (int)socket;

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (!fgets(buffer, (int)sizeof(buffer), stdin))
            error("ERROR reading stdin");

        ssize_t n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing to socket");
    }
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n, ret;
    pthread_t rThread, sThread;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    while (1)
    {
        // creating a new thread for sending messages to the client
        if (ret = pthread_create(&sThread, NULL, sendMessage, (void *)newsockfd))
        {
            printf("ERROR: Return Code from pthread1_create() is %d\n", ret);
            error("ERROR creating thread");
        }

        // creating a new thread for receiving messages from the client
        if (ret = pthread_create(&rThread, NULL, receiveMessage, (void *)newsockfd))
        {
            printf("ERROR: Return Code from pthread2_create() is %d\n", ret);
            error("ERROR creating thread");
        }
    }

    return 0;
}