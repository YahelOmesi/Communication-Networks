#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#ifndef TCP_CONGESTION
#define TCP_CONGESTION 13
#endif

#define FILE_SIZE 2097152 // 2MB

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        printf("Usage: %s -ip <IP> -p <PORT> -algo <ALGO> \n", argv[0]);
        return 1;
    }

    char *ip;
    int port;
    char *algo;

    // char *file_name = argv[4];
    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            port = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-algo") == 0)
        {
            algo = argv[i + 1];
        }
        else if (strcmp(argv[i], "-ip") == 0)
        {
            ip = argv[i + 1];
        }
        else
        {
            printf("Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    // Open file
    FILE *file = fopen("testfile.bin", "rb");
    if (file == NULL)
    {
        printf("Error opening file");
        return 1;
    }

    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // ipv4, tcp
    if (sockfd == -1)
    {
        printf("Socket creation failed");
        return 1;
    }

    // Set congestion control algorithm
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) != 0)
    {
        printf("Setting congestion control algorithm failed");
        return 1;
    }

    // Connect to receiver
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        printf("Connection failed");
        return 1;
    }

    int command;
    char buffer[FILE_SIZE];
    size_t bytes_read;

    while (1)
    {
        printf("Press 1 to send or 2 to exit\n");
        scanf("%d", &command);

        if (1 == command)
        {
            send(sockfd, "F", 1, 0);
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
            {
                if (send(sockfd, buffer, bytes_read, 0) == -1)
                {
                    printf("Sending file failed");
                    return 1;
                }
            }
            send(sockfd, NULL, 0, 0);
            fseek(file, 0, SEEK_SET);
        }
        else if (2 == command)
        {
            send(sockfd, "Q", 1, 0);
            break;
        }
        getchar();
    }

    // Close socket and file
    fclose(file);
    close(sockfd);

    return 0;
}