#include "server.h"

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t addr_size;

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Bind socket to the specified IP and port
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for client connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("CDR Nexus: User Authentication Server listening on port %d\n", PORT);

    // Accept and handle client connections
    while (1) {
        addr_size = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &addr_size);

        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        printf("Client connected.\n");

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handleClientConnection, (void*)&client_socket);
    }
    close(server_socket);
    return EXIT_SUCCESS;
}
