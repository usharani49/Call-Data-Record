#include "../include/client.h"
#include "../include/client_operations.h"
#include "../include/network_utils.h"

int main() {
    int sockfd;
    struct sockaddr_in server_address;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection to server failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    displayMenu(sockfd);

    close(sockfd);
    return EXIT_SUCCESS;
}
