#include "../include/client.h"
#include "../include/network_utils.h"

#define MAX_LENGTH 100

void sendCredentials(int sockfd, const char* username, const char* password, const char* confirm_password) {
    send(sockfd, username, MAX_LENGTH, 0);
    send(sockfd, password, MAX_LENGTH, 0);
    if (confirm_password != NULL) {
        send(sockfd, confirm_password, MAX_LENGTH, 0);
    }
}

void receiveResponse(int sockfd) {
    char response[MAX_LENGTH];
    memset(response, '\0', sizeof(response));
    if (recv(sockfd, response, sizeof(response), 0) > 0) {
        printf("Server response: %s\n", response);
    }
}
