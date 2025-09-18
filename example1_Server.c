#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

char username[32];
char remoteUsername[32];

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int checkExitMsg(char *msg)
{
    size_t len = strlen(msg);
    if (len > 0 && msg[len - 1] == '\n')
    {
        msg[len - 1] = '\0';
    }
    if ((strcmp(msg, "exit") == 0))
    {
        return 1;
    }

    return 0;
}

void *receiveMessage(void *socket)
{
    int sockfd = *(int *)socket;
    char buffer[256];
    int ret;

    memset(buffer, 0, sizeof(buffer));
    while ((ret = read(sockfd, buffer, sizeof(buffer) - 1)) > 0)
    {
        printf("message received");
        buffer[ret] = '\0';
        if (checkExitMsg(buffer))
        {
            exit(1);
        }
        printf("%s", buffer);
    }
    if (ret < 0)
        printf("Error receiving data!\n");
    else
        printf("Closing connection\n");
    close(sockfd);
    exit(1);
    return NULL;
}

void *sendMessage(void *socket)
{
    int sockfd = *(int *)socket;
    char buffer[256];
    ssize_t n;

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (!fgets(buffer, sizeof(buffer), stdin))
            error("ERROR reading stdin");

        char message[300];
        snprintf(message, sizeof(message), "<%s> %s", username, buffer);
        
        if(checkExitMsg(buffer))
        {
            n = write(sockfd, buffer, strlen(buffer));
            exit(1);
        }
        else
        {
            n = write(sockfd, message, strlen(message));
        }

        if (n < 0)
            error("ERROR writing to socket");
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t rThread, sThread;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    printf("Provide user name: ");
    if (!fgets(username, sizeof(username), stdin))
        error("ERROR reading username");
    username[strcspn(username, "\n")] = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    read(newsockfd, remoteUsername, sizeof(remoteUsername));
    write(newsockfd, username, strlen(username));
    printf("Connection established with %s (%s)\n",
           inet_ntoa(cli_addr.sin_addr), remoteUsername);

    if (pthread_create(&sThread, NULL, sendMessage, &newsockfd))
        error("ERROR creating send thread");

    if (pthread_create(&rThread, NULL, receiveMessage, &newsockfd))
        error("ERROR creating receive thread");

    pthread_join(sThread, NULL);
    pthread_join(rThread, NULL);

    return 0;
}