#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>   // For socket(), bind(), listen(), accept(), etc.
#include <netinet/in.h>    // For sockaddr_in, htons(), INADDR_ANY
#include <arpa/inet.h>     // For inet_pton(), inet_ntoa() (if needed)

#define MAX_TEXT 10000
#define PORT 8080
#define BUFFER_SIZE 1024

// Function to extract text from a PDF file using pdftotext
char *extract_text_from_pdf(const char *pdf_filename, int *size) {
    char command[1024];
    char temp_filename[] = "/tmp/extracted_text.txt";
    
    // Call pdftotext to extract the text to a temporary file
    snprintf(command, sizeof(command), "pdftotext \"%s\" \"%s\"", pdf_filename, temp_filename);
    
    // Execute the command to extract text
    int result = system(command);
    if (result != 0) {
        perror("Failed to extract text from PDF");
        exit(EXIT_FAILURE);
    }

    // Open the extracted text file
    FILE *file = fopen(temp_filename, "r");
    if (!file) {
        perror("Failed to open the extracted text file");
        exit(EXIT_FAILURE);
    }

    // Read the content into memory
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    char *content = (char *)malloc(*size + 1);
    if (!content) {
        perror("Memory allocation failed");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fread(content, 1, *size, file);
    content[*size] = '\0';  // Null-terminate the string
    fclose(file);
    return content;
}

// Function to compare substrings and detect plagiarism
void detect_plagiarism(const char *file1_content, int file1_size, 
                       const char *file2_content, int file2_size, int substring_length) {
    if (substring_length <= 0 || substring_length > file1_size || substring_length > file2_size) {
        printf("Invalid substring length!\n");
        return;
    }

    int matches = 0, total_comparisons = 0;
    char plagiarized_content[BUFFER_SIZE * 2] = "";  // Buffer to store plagiarized content
    int line_number = 1;
    int is_plagiarized = 0;  // Flag to track if content is plagiarized

    // Compare substrings
    for (int i = 0; i <= file1_size - substring_length; i++) {
        char substring1[substring_length + 1];
        strncpy(substring1, &file1_content[i], substring_length);
        substring1[substring_length] = '\0';

        for (int j = 0; j <= file2_size - substring_length; j++) {
            char substring2[substring_length + 1];
            strncpy(substring2, &file2_content[j], substring_length);
            substring2[substring_length] = '\0';

            if (strcmp(substring1, substring2) == 0) {
                matches++;

                // Add the matched content with the surrounding context (from both files)
                if (!is_plagiarized) {
                    // We add the plagiarism information only once
                    snprintf(plagiarized_content, sizeof(plagiarized_content), 
                             "Plagiarized content found in both files:\n%s\n---\n%s",
                             &file1_content[i], &file2_content[j]);
                    is_plagiarized = 1;  // Mark as plagiarized to avoid printing again
                }
                break;  // Exit the inner loop once a match is found
            }
        }
        total_comparisons++;
        if (file1_content[i] == '\n') {
            line_number++;
        }
    }

    // Display plagiarism percentage and content
    float plagiarism_percentage = total_comparisons ? ((float)matches / total_comparisons) * 100 : 0;
    printf("Plagiarism Percentage: %.2f%%\n", plagiarism_percentage);

    if (matches > 0) {
        printf("Plagiarized Content:\n%s\n", plagiarized_content);
    } else {
        printf("No plagiarism detected.\n");
    }
}

// Server setup and communication
void start_server() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept client connections and process requests
    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected\n");

        // Receive request from client (file1, file2)
        int received_bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (received_bytes <= 0) {
            perror("Failed to receive data");
            close(client_fd);
            continue;
        }

        buffer[received_bytes] = '\0';
        printf("Received request: %s\n", buffer);

        // Parse the client request (file1, file2)
        char file1[256], file2[256];
        sscanf(buffer, "%255s %255s", file1, file2);

        // Ask the server user for the substring matching size
        int substring_length;
        printf("Enter the substring length for plagiarism detection: ");
        if (scanf("%d", &substring_length) != 1) {
            printf("Invalid input. Exiting...\n");
            close(client_fd);
            continue;
        }

        // Read the files
        int file1_size, file2_size;
        char *file1_content = extract_text_from_pdf(file1, &file1_size);
        char *file2_content = extract_text_from_pdf(file2, &file2_size);

        // Perform plagiarism check
        detect_plagiarism(file1_content, file1_size, file2_content, file2_size, substring_length);

        // Send response to the client (simple confirmation for now)
        char response[] = "Plagiarism check completed.";
        send(client_fd, response, strlen(response), 0);

        // Clean up
        free(file1_content);
        free(file2_content);
        close(client_fd);
    }

    close(server_fd);
}

int main() {
    start_server();
    return 0;
}

