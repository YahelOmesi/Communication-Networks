#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <netinet/tcp.h>
#include <sys/time.h>

#define BUFFER_SIZE 4096

double calculate_time(struct timespec *start, struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_nsec - start->tv_nsec) / 1000000000;
}

double recive_file(int sockfd)
{

    // Receive file
    char buffer[BUFFER_SIZE];
    FILE *file = tmpfile();
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // We will enter the current time at the start of the process
    ssize_t bytes_received;
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, 1, bytes_received, file); // writes the buffer in parts into the fill
    }
    clock_gettime(CLOCK_MONOTONIC, &end); // We will enter the current time at the end of the process

    // Calculate time
    double elapsed_time = calculate_time(&start, &end);

    fclose(file);
    return elapsed_time;
}

int main(int argc, char *argv[])
{
    int file_count = 0;
    double total_time = 0;
    double elapsed_time[10] = {0};

    if (argc != 5)
    {
        printf("Usage: %s -p <PORT> -algo <ALGO>\n", argv[0]);
        return 1;
    }

    int port;
    char *algo;

    // We will use the "-p" and "-algo" indicators so that the program will work properly
    // even when the user entered inputs in the order of his choice

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
        else
        {
            printf("Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    printf("Starting Receiver...\n");

    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // ipv4,tcp
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        return 1;
    }

    // Set congestion control algorithm
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) != 0)
    {
        printf("Setting congestion control algorithm failed");
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // Bind socket
    struct sockaddr_in serv_addr, cli_addr;
    serv_addr.sin_family = AF_INET; // ipv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port); // socket port
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        printf("Binding failed");
        return 1;
    }

    // now the socket ready to operation

    printf("Waiting for TCP connection on port %d...\n", port);

    // Listen for connections
    if (listen(sockfd, 1) != 0)
    {
        printf("Listening failed");
        return 1;
    }

    // now the socket is ready to accept requests. for this to happen you need to do accept.

    // Accept connection
    socklen_t cli_len = sizeof(cli_addr);                                   //;
    int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len); // the client socket number
    if (newsockfd == -1)
    {
        printf("Accepting connection failed");
        return 1;
    }
    printf("Sender connected\n");

    if (setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof timeout) < 0)
        printf("setsockopt failed\n");

    char command;

    while (1)
    {
        recv(newsockfd, &command, sizeof(command), 0);
        if ('Q' == command)
        {
            break;
        }

        else if ('F' == command)
        {
            elapsed_time[file_count] = recive_file(newsockfd);
            total_time += elapsed_time[file_count++];
            printf("File accepted\n");
        }

        command = 0;
    }

    // Print statistics
    printf("- * Statistics * -\n");
    for (int i = 0; i < file_count; i++)
    {
        printf("----------------------------------\n");
        printf("- Run #%d Data: Time=%.2fms; Speed=%.2fMB/s\n", i, elapsed_time[i] * 1000, (2.0 / elapsed_time[i]));
        printf("----------------------------------\n");
    }

    printf("- Average time: %.2fms\n", (total_time / file_count) * 1000);
    printf("- Average bandwidth: %.2fMB/s\n", (2.0 / (total_time / file_count)));
    printf("----------------------------------\n");
    printf("Sender sent exit message.\n");
    printf("Receiver end.\n");

    // Close sockets and file
    close(newsockfd);
    close(sockfd);

    return 0;
}