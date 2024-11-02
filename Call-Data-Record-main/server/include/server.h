#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_LENGTH 100
#define PORT 4005
#define MAX_CLIENTS 10
#define USER_DATA_FILE "users.txt"
#define CUSTOMER_CDR_DATA_FILE "CB.txt"
#define OPERATOR_CDR_DATA_FILE "IOSB.txt"
#define BUFFER_SIZE 1024
#define LARGE_BUFFER_SIZE 100001

// User action codes
#define USER_ACTION_SIGNUP 1
#define USER_ACTION_LOGIN 2
#define USER_ACTION_EXIT 3
#define USER_ACTION_PROCESS_CDR 4
#define USER_ACTION_BILLING_INFO 5
#define USER_ACTION_LOGOUT 6
#define USER_BILLING_ACTION_CUSTOMER 7
#define USER_BILLING_ACTION_INTEROPERATOR 8

typedef struct {
    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
} User;

void *handleClientConnection(void *client_socket);
int isUserAllowed(const char *username, const char *password);
int isUserRegistered(const char *username);
void registerUser(int client_socket);
void authenticateUser(int client_socket);
void receiveCredentials(int client_socket, User *user, int isSignup);
void sendResponse(int client_socket, const char *response);
void lockMutex();
void unlockMutex();
void processCDR(int client_socket);
void billingInfo(int client_socket);

#endif // SERVER_H
