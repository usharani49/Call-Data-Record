#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

void sendCredentials(int sockfd, const char* username, const char* password, const char* confirm_password);
void receiveResponse(int sockfd);

#endif
