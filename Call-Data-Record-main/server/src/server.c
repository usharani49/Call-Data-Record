#include "server.h"
#include "user_management.h"
#include "billing.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handleClientConnection(void *client_socket) {
    int socket_fd = *(int*)client_socket;
    int user_action;

    while (1) {
        if (recv(socket_fd, &user_action, sizeof(user_action), 0) <= 0) {
            printf("Client disconnected.\n");
            close(socket_fd);
            pthread_exit(NULL);
        }
        printf("Received action: %d\n", user_action);
        switch (user_action) {
            case USER_ACTION_SIGNUP:
                printf("Client chose to sign up.\n");
                registerUser(socket_fd);
                break;
            case USER_ACTION_LOGIN:
                printf("Client chose to log in.\n");
                authenticateUser(socket_fd);
                break;
            case USER_ACTION_EXIT:
                printf("Client chose to exit.\n");
                close(socket_fd);
                pthread_exit(NULL);
            default:
                printf("Invalid choice from client.\n");
        }
    }
}

void registerUser(int client_socket) {
    User new_user;
    char confirm_password[MAX_LENGTH];
    receiveCredentials(client_socket, &new_user, 1);
    recv(client_socket, confirm_password, MAX_LENGTH, 0);
    lockMutex();
    if (strlen(new_user.username) == 0 || strlen(new_user.password) == 0) {
        sendResponse(client_socket, "Error: Username and password cannot be empty.");
    } else if (isUserRegistered(new_user.username)) {
        sendResponse(client_socket, "Error: Username already exists. Please choose a different username.");
    } else if (strcmp(new_user.password, confirm_password) != 0) {
        sendResponse(client_socket, "Error: Passwords do not match. Please try again.");
    } else {
        FILE *file = fopen(USER_DATA_FILE, "a");
        if (file) {
            fprintf(file, "%s %s\n", new_user.username, new_user.password);
            fclose(file);
            sendResponse(client_socket, "Registration successful! Please log in.");
        } else {
            sendResponse(client_socket, "Error: Could not open user data file.");
        }
    }
    unlockMutex();
}

void authenticateUser(int client_socket) {
    User login_user;
    receiveCredentials(client_socket, &login_user, 0);
    lockMutex();
    if (!isUserRegistered(login_user.username)) {
        sendResponse(client_socket, "Error: User not registered. Please sign up first.");
        unlockMutex();
        return;
    }

    for (int attempts = 0; attempts < 3; attempts++) {
        if (isUserAllowed(login_user.username, login_user.password)) {
            sendResponse(client_socket, "Login successful!");
            int post_login_action;
            while (1) {
                recv(client_socket, &post_login_action, sizeof(post_login_action), 0);

                switch (post_login_action) {
                    case USER_ACTION_PROCESS_CDR:
                        processCDR(client_socket);
                        break;
                    case USER_ACTION_BILLING_INFO:
                        billingInfo(client_socket);
                        break;
                    case USER_ACTION_LOGOUT:
                        sendResponse(client_socket, "Logout successful.");
                        unlockMutex();
                        return;
                    default:
                        sendResponse(client_socket, "Invalid choice.");
                }
            }
        } else {
            if (attempts < 2) {
                sendResponse(client_socket, "Error: Incorrect password. Please try again.");
                receiveCredentials(client_socket, &login_user, 0);
            } else {
                sendResponse(client_socket, "Error: Too many failed attempts. Please try again later.");
            }
        }
    }
    unlockMutex();
}

int isUserRegistered(const char *username) {
    FILE *file = fopen(USER_DATA_FILE, "r");
    char stored_username[MAX_LENGTH];
    if (!file) return 0;
    while (fscanf(file, "%s", stored_username) != EOF) {
        if (strcmp(stored_username, username) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int isUserAllowed(const char *username, const char *password) {
    FILE *file = fopen(USER_DATA_FILE, "r");
    char stored_username[MAX_LENGTH], stored_password[MAX_LENGTH];
    if (!file) return 0;
    while (fscanf(file, "%s %s", stored_username, stored_password) != EOF) {
        if (strcmp(stored_username, username) == 0) {
            fclose(file);
            return strcmp(stored_password, password) == 0;
        }
    }
    fclose(file);
    return 0;
}

void receiveCredentials(int client_socket, User *user, int isSignup) {
    recv(client_socket, user->username, MAX_LENGTH, 0);
    recv(client_socket, user->password, MAX_LENGTH, 0);
}

void sendResponse(int client_socket, const char *response) {
    send(client_socket, response, strlen(response) + 1, 0);
}

void lockMutex() {
    pthread_mutex_lock(&mutex);
}

void unlockMutex() {
    pthread_mutex_unlock(&mutex);
}

void processCDR(int client_socket) {
    sendResponse(client_socket, "Processing CDR files...");
    // Implement actual CDR file processing logic here.
}

void billingInfo(int client_socket) {
    int billingType;
    char response[MAX_LENGTH];
    recv(client_socket, &billingType, sizeof(billingType), 0);
    switch (billingType) {
        case USER_BILLING_ACTION_CUSTOMER:
            customerBilling(client_socket);
            break;
        case USER_BILLING_ACTION_INTEROPERATOR:
            interoperatorBilling(client_socket);
            break;
        default:
            strcpy(response, "Invalid Billing Choice option.");
            send(client_socket, response, strlen(response) + 1, 0);
    }
}
