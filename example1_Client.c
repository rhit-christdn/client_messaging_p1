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

char username[32];
char remoteUsername[32];

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int checkExitMsg(char *msg) {
    return strcmp(msg, "exit") == 0;
}

void *receiveMessage(void *socket) {
    int sockfd = *(int *)socket;
    char buffer[256];
    int ret;

    while ((ret = read(sockfd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[ret] = '\0';

        if (checkExitMsg(buffer)) {
            printf("Remote user exited.\n");
            close(sockfd);
            exit(0);
        }

        printf("%s\n", buffer);
        fflush(stdout);
    }

    if (ret < 0)
        perror("Error receiving data");
    else
        printf("Connection closed by remote\n");

    close(sockfd);
    exit(0);
    return NULL;
}

void *sendMessage(void *socket) {
    int sockfd = *(int *)socket;
    char buffer[256];
    ssize_t n;

    while (1) {
        if (!fgets(buffer, sizeof(buffer), stdin))
            error("ERROR reading stdin");

        buffer[strcspn(buffer, "\n")] = '\0';

        if (checkExitMsg(buffer)) {
            write(sockfd, "exit", 4);
            printf("Exiting...\n");
            close(sockfd);
            exit(0);
        }

        char message[300];
        snprintf(message, sizeof(message), "<%s> %s", username, buffer);

        n = write(sockfd, message, strlen(message));
        if (n < 0)
            error("ERROR writing to socket");
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t rThread, sThread;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
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

    write(sockfd, username, strlen(username));
    read(sockfd, remoteUsername, sizeof(remoteUsername));

    printf("Connected to server as %s, remote is %s\n", username, remoteUsername);

    if (pthread_create(&sThread, NULL, sendMessage, &sockfd))
        error("ERROR creating send thread");

    if (pthread_create(&rThread, NULL, receiveMessage, &sockfd))
        error("ERROR creating receive thread");

    pthread_join(sThread, NULL);
    pthread_join(rThread, NULL);

    close(sockfd);
    return 0;
}