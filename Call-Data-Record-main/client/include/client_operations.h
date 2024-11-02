#ifndef CLIENT_OPERATIONS_H
#define CLIENT_OPERATIONS_H

void displayMenu(int sockfd);
void signUp(int sockfd);
void logIn(int sockfd);
void postLoginMenu(int sockfd);
void customerBilling(int sockfd);
void interoperatorBilling(int sockfd);
void exitProgram(int sockfd);

#endif
