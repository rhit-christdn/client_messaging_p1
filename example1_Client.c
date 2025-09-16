#include <stdio.h>
#include <stdlib.h>     // exit, atoi
#include <string.h>     // strlen, memset, memcpy
#include <unistd.h>     // read, write, close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // gethostbyname
#include <arpa/inet.h>  // htons (often in netinet/in.h, but safe to include)

static void error(const char *msg)
{
    perror(msg);
    exit(1);
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

void *receiveMessage(void *socket)
{
    int sockfd, ret;
    char buffer[256];
    sockfd = (int)socket;

    memset(buffer, 0, sizeof(buffer));
    while ((ret = read(sockfd, buffer, sizeof(buffer))) > 0)
    {
        printf("<client>: %s", buffer);
    }
    if (ret < 0)
        printf("Error receiving data!\n");
    else
        printf("Closing connection\n");
    close(sockfd);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, ret;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    pthread_t rThread, sThread;

    if (argc < 3) {
        fprintf(stderr, "usage: %s hostname port\n", argv[0]);
        return 1;
    }

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

    while (1)
    {
        // creating a new thread for sending messages to the server
        if (ret = pthread_create(&sThread, NULL, sendMessage, (void *)sockfd))
        {
            printf("ERROR: Return Code from pthread2_create() is %d\n", ret);
            error("ERROR creating thread");
        }

        // creating a new thread for receiving messages from the server
        if (ret = pthread_create(&rThread, NULL, receiveMessage, (void *)sockfd))
        {
            printf("ERROR: Return Code from pthread1_create() is %d\n", ret);
            error("ERROR creating thread");
        }
    }

    close(sockfd);
    return 1;
}
