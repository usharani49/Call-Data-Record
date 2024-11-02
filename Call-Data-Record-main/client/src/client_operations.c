#include "../include/client.h"
#include "../include/client_operations.h"
#include "../include/network_utils.h"

// User actions
#define USER_ACTION_SIGNUP 1
#define USER_ACTION_LOGIN 2
#define USER_ACTION_EXIT 3

// Post-login actions
#define POST_LOGIN_ACTION_PROCESS_CDR 4
#define POST_LOGIN_ACTION_BILLING_INFO 5
#define POST_LOGIN_ACTION_LOGOUT 6

// Billing actions
#define BILLING_ACTION_CUSTOMER 7
#define BILLING_ACTION_INTEROPERATOR 8

void displayMenu(int sockfd) {
    int choice;

    while (1) {
        printf("\n--- CDR Nexus User Authentication ---\n");
        printf("1. Sign Up\n");
        printf("2. Log In\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // Consume newline

        switch (choice) {
            case USER_ACTION_SIGNUP:
                signUp(sockfd);
                break;
            case USER_ACTION_LOGIN:
                logIn(sockfd);
                break;
            case USER_ACTION_EXIT:
                exitProgram(sockfd);
                return;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}

void signUp(int sockfd) {
    char username[MAX_LENGTH], password[MAX_LENGTH], confirm_password[MAX_LENGTH];

    printf("Enter username: ");
    fgets(username, MAX_LENGTH, stdin);
    username[strcspn(username, "\n")] = '\0';

    printf("Enter password: ");
    fgets(password, MAX_LENGTH, stdin);
    password[strcspn(password, "\n")] = '\0';

    printf("Re-enter password: ");
    fgets(confirm_password, MAX_LENGTH, stdin);
    confirm_password[strcspn(confirm_password, "\n")] = '\0';

    int action = USER_ACTION_SIGNUP;
    send(sockfd, &action, sizeof(action), 0);
    sendCredentials(sockfd, username, password, confirm_password);
    receiveResponse(sockfd);
}

void logIn(int sockfd) {
    char username[MAX_LENGTH], password[MAX_LENGTH];

    printf("Enter username: ");
    fgets(username, MAX_LENGTH, stdin);
    username[strcspn(username, "\n")] = '\0';

    int action = USER_ACTION_LOGIN;
    send(sockfd, &action, sizeof(action), 0);

    for (int attempts = 0; attempts < 3; attempts++) {
        printf("Enter password: ");
        fgets(password, MAX_LENGTH, stdin);
        password[strcspn(password, "\n")] = '\0';

        sendCredentials(sockfd, username, password, NULL);

        char response[MAX_LENGTH];
        memset(response, '\0', sizeof(response));
        if (recv(sockfd, response, sizeof(response), 0) > 0) {
            printf("%s\n", response);
            fflush(stdout);

            if (strcmp(response, "Login successful!") == 0) {
                postLoginMenu(sockfd);
                return;
            } else {
                printf("Invalid login attempt. Please try again.\n");
                if (attempts == 2) {
                    printf("Too many failed attempts. Please try again later.\n");
                }
            }
        }
    }
}

void postLoginMenu(int sockfd) {
    int post_login_choice;
	int cdrProcessed = 0; //flag initialization

    while (1) {
        printf("\n--- Post Login Menu ---\n");
        printf("4. Process CDR file\n");
        printf("5. Billing Information\n");
        printf("6. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &post_login_choice);
        send(sockfd, &post_login_choice, sizeof(post_login_choice), 0);
        getchar(); // Consume newline
		if (post_login_choice == POST_LOGIN_ACTION_LOGOUT) {
            printf("Logging out...\n");
            break;
        }

        if (post_login_choice == POST_LOGIN_ACTION_BILLING_INFO) {
            // Check if it's the first time entering Billing Information
            if (cdrProcessed == 0) {
                printf("First Process CDR files.\n");
                cdrProcessed = 1; // Update the flag to indicate CDR processing
                continue; // Go back to the post-login menu
            }

            int billing_choice;
            printf("\n--- Billing Menu ---\n");
            printf("7. Customer Billing\n");
            printf("8. Interoperator Settlement Billing\n");
            printf("Enter your choice: ");
            scanf("%d", &billing_choice);
            send(sockfd, &billing_choice, sizeof(billing_choice), 0);

            switch (billing_choice) {
                case BILLING_ACTION_CUSTOMER:
                    customerBilling(sockfd);
                    break;
                case BILLING_ACTION_INTEROPERATOR:
                    interoperatorBilling(sockfd);
                    break;
                default:
                    printf("Invalid billing choice.\n");
            }
        } else if (post_login_choice == POST_LOGIN_ACTION_LOGOUT) {
            printf("Logging out...\n");
            break;
        } else if (post_login_choice == POST_LOGIN_ACTION_PROCESS_CDR) {
            printf("Processing CDR file...\n");
            // Implement CDR processing logic here
        } else {
            printf("Invalid choice. Please try again.\n");
        }
    }
}

void customerBilling(int sock) {
    int billingChoice;
    char msisdn[MAX_LENGTH];
    printf("\n--- Customer Billing Menu ---\n");
    printf("9. Search for MSISDN\n");
    printf("10. Get all processed data (CB.txt)\n");
    printf("Enter your choice: ");
    scanf("%d", &billingChoice);
    send(sock, &billingChoice, sizeof(billingChoice), 0);
    if (billingChoice == 9) {
        printf("Enter the MSISDN: ");
        scanf("%s",msisdn);
        msisdn[strcspn(msisdn, "\n")] = '\0';
        send(sock, msisdn, MAX_LENGTH, 0);

        char response[1024];
        memset(response, '\0', sizeof(response));
        if (recv(sock, response, sizeof(response), 0) > 0) {
            printf("Server response: %s\n", response);
        }
    } else if (billingChoice == 10) {
        char response[MAX_LENGTH];
        if (recv(sock, response, sizeof(response), 0) > 0) {
            printf("Server response: %s\n", response);
        }
    }
}

void interoperatorBilling(int sock) {
    int billingChoice;
    char operatorID[MAX_LENGTH];

    printf("\n--- Interoperator Settlement Billing Menu ---\n");
    printf("11. Search for Brand Name/Operator ID\n");
    printf("12. Get all processed data (IOSB.txt)\n");
    printf("Enter your choice: ");
    scanf("%d", &billingChoice);
    send(sock, &billingChoice, sizeof(billingChoice), 0);
    if (billingChoice == 11) {
        printf("Enter the Operator ID or Brand Name: ");
        scanf("%s",operatorID);
        operatorID[strcspn(operatorID, "\n")] = '\0'; // Remove newline character
        send(sock, operatorID, MAX_LENGTH, 0);

        char response[1024];
        memset(response, '\0', sizeof(response));
        if (recv(sock, response, sizeof(response), 0) > 0) {
            printf("Server response: %s\n", response);
        }
    } else if (billingChoice == 12) {
        char response[MAX_LENGTH];
        if (recv(sock, response, sizeof(response), 0) > 0) {
            printf("Server response: %s\n", response);
        }
    }
}

void exitProgram(int sockfd) {
    int action = USER_ACTION_EXIT;
    send(sockfd, &action, sizeof(action), 0);
    printf("Exiting the program...\n");
}
