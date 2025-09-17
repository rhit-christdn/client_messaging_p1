#include <stdio.h>
#include <stdlib.h>     // exit, atoi
#include <string.h>     // strlen, memset, memcpy
#include <unistd.h>     // read, write, close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // gethostbyname
#include <arpa/inet.h>  // htons, inet_ntoa
#include <pthread.h>    // pthreads

char username[32];
char remoteUsername[32];

static void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void *sendMessage(void *socket)
{
    int sockfd = *(int *)socket;
    char buffer[256];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (!fgets(buffer, sizeof(buffer), stdin))
            error("ERROR reading stdin");

        char message[300];
        snprintf(message, sizeof(message), "<%s> %s", username, buffer);

        ssize_t n = write(sockfd, message, strlen(message));
        if (n < 0)
            error("ERROR writing to socket");
    }
    return NULL;
}

void *receiveMessage(void *socket)
{
    int sockfd = *(int *)socket;
    char buffer[256];
    int ret;

    memset(buffer, 0, sizeof(buffer));
    while ((ret = read(sockfd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[ret] = '\0';
        printf("%s", buffer);
    }
    if (ret < 0)
        printf("Error receiving data!\n");
    else
        printf("Closing connection\n");
    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t rThread, sThread;

    if (argc < 3) {
        fprintf(stderr, "usage: %s hostname port\n", argv[0]);
        return 1;
    }

    printf("Provide user name: ");
    if (!fgets(username, sizeof(username), stdin))
        error("ERROR reading username");
    username[strcspn(username, "\n")] = 0;

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

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // send our username first, receive remote username
    write(sockfd, username, strlen(username));
    read(sockfd, remoteUsername, sizeof(remoteUsername));
    printf("Connected to server as %s, remote is %s\n", username, remoteUsername);

    // create threads ONCE
    if (pthread_create(&sThread, NULL, sendMessage, &sockfd))
        error("ERROR creating send thread");

    if (pthread_create(&rThread, NULL, receiveMessage, &sockfd))
        error("ERROR creating receive thread");

    pthread_join(sThread, NULL);
    pthread_join(rThread, NULL);

    close(sockfd);
    return 0;
}
