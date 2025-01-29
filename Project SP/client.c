#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_TEXT 10000
#define PORT 8080
#define BUFFER_SIZE 1024

void start_client() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Assuming the server is on the same machine for testing

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Get file names from the user
    char file1[256], file2[256];
    printf("Enter path of the first file: ");
    scanf("%s", file1);
    printf("Enter path of the second file: ");
    scanf("%s", file2);

    // Send request to server
    snprintf(buffer, sizeof(buffer), "%s %s", file1, file2);
    send(sockfd, buffer, strlen(buffer), 0);

    // Receive response from server
    int received_bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (received_bytes > 0) {
        buffer[received_bytes] = '\0';
        printf("Server response: %s\n", buffer);
    }

    close(sockfd);
}

int main() {
    start_client();
    return 0;
}

