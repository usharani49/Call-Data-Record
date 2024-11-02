/*******************************************************************************************************************************************************
  * FILE NAME : Server.c
  *
  * DESCRIPTION : This file contains the server-side implementation for handling user signup and login.
  * The server now provides additional functionalities for processing CDR files and billing information.
  *
  * SERVER NAME : CDR Nexus
  ******************************************************************************************************************************************************/

// In-built header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

// User defined MACROS
#define MAX_LENGTH 100 // Maximum length for usernames and passwords
#define PORT 5642 // Port number for the server
#define MAX_CLIENTS 10 // Maximum number of clients that can connect simultaneously
#define USER_DATA_FILE "users.txt" // File to store user credentials
#define BUFFER_SIZE 1024 //to store response
#define ENTRIES 100001 //to know data of how many users

// User action MACROS
#define USER_ACTION_SIGNUP 1 // Action for user signup
#define USER_ACTION_LOGIN 2 // Action for user login
#define USER_ACTION_EXIT 3 // Action for user exit
#define USER_ACTION_PROCESS_CDR 4 // Action to process CDR file after login
#define USER_ACTION_BILLING_INFO 5 // Action to print/search billing information after login
#define USER_ACTION_LOGOUT 6 // Action to logout after login
#define USER_BILLING_ACTION_CUSTOMER 7 // Action for customer billing
#define USER_BILLING_ACTION_INTEROPERATOR 8 // Action for interoperator billing
#define USER_BILLING_ACTION_MSISDN 9 // Action for entering msisdn in customer billing
#define USER_BILLING_ACTION_CSFILE 10 // Action for pushing details in a file in customer billing
#define USER_BILLING_ACTION_OPID 11 // Action for entering operator id in interoperator billing
#define USER_BILLING_ACTION_OPFILE 12 // Action for pushing details in a file in interoperator billing

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread safety
typedef struct {
    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
} User;
// Structure to hold a single operator data
typedef struct {
    char brand[65];
    char mmc[7];
    long incoming_call_duration;
    long outgoing_call_duration;
    long incoming_sms;
    long outgoing_sms;
    long mb_downloaded;
    long mb_uploaded;
} OperatorData;
 // Structure to hold a single customer data
typedef struct {
    char msisdn[20];
    char brand[65];
    char mmc[7];
    long incoming_call_duration_within;
    long outgoing_call_duration_within;
    long incoming_sms_within;
    long outgoing_sms_within;
    long incoming_call_duration_outside;
    long outgoing_call_duration_outside;
    long incoming_sms_outside;
    long outgoing_sms_outside;
    long mb_downloaded;
    long mb_uploaded;
} CustomerData;

typedef struct {
    char brand[50];
    char id[10];
    CustomerData data;
} CustomerSummary;

 // Structure to hold the operators data
typedef struct {
    char brand[50];
    char id[10];
    OperatorData data;
} OperatorSummary;
 // Function Prototypes
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
void customerBilling(int client_socket);
void process_customer_by_id(const char *filename, const char *msisdn, CustomerData *data);
int find_or_create_customer(CustomerSummary **customers, int *customer_count, const char *brand, const char *id);
void process_customer_whole(const char *filename, CustomerSummary **customers);
void process_interoperator_by_id(const char *filename, const char *mmc, OperatorData *data);
int find_or_create_operator(OperatorSummary **operators, int *operator_count, const char *brand, const char *id);
void process_interoperator_whole(const char *filename, OperatorSummary **operators);
void init_customer_data(CustomerData *op_data);
void init_operator_data(OperatorData *op_data);
void interoperatorBilling(int client_socket);
// Main function
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

// Function to handle client requests (Signup, Login)
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

 // Function to register a new user
 void registerUser(int client_socket) {
     User new_user;
     char confirm_password[MAX_LENGTH];
     // Receive the user's credentials (username and password)
     receiveCredentials(client_socket, &new_user, 1); // Receive signup credentials
     recv(client_socket, confirm_password, MAX_LENGTH, 0); // Receive confirmation password
     lockMutex(); // Lock mutex for thread safety
     // Check if username is empty
     if (strlen(new_user.username) == 0 || strlen(new_user.password) == 0) {
         sendResponse(client_socket, "Error: Username and password cannot be empty.");
     } else if (isUserRegistered(new_user.username)) {
         sendResponse(client_socket, "Error: Username already exists. Please choose a different username.");
     } else if (strcmp(new_user.password, confirm_password) != 0) { // Check if passwords match
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
     unlockMutex(); // Unlock mutex
 }

 // Function to authenticate an existing user
 void authenticateUser(int client_socket) {
     User login_user;
     receiveCredentials(client_socket, &login_user, 0); // Receive login credentials
     lockMutex(); // Lock mutex for thread safety
     // Check if the user is registered
     if (!isUserRegistered(login_user.username)) {
         sendResponse(client_socket, "Error: User not registered. Please sign up first.");
         unlockMutex(); // Unlock mutex
         return; // Exit function
     }

     // Allow the user to attempt login multiple times
     for (int attempts = 0; attempts < 3; attempts++) {
         if (isUserAllowed(login_user.username, login_user.password)) {
             sendResponse(client_socket, "Login successful!");
             int post_login_action;
             while (1) { // Post-login menu loop
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
             if (attempts < 2) { // Provide feedback for failed attempts
                 sendResponse(client_socket, "Error: Incorrect password. Please try again.");
                 receiveCredentials(client_socket, &login_user, 0); // Receive credentials again
             } else {
                 sendResponse(client_socket, "Error: Too many failed attempts. Please try again later.");
             }
         }
     }
     unlockMutex(); // Unlock mutex
 }

 // Function to check if the user is registered
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

 // Function to check if the username and password match the record
 int isUserAllowed(const char *username, const char *password) {
     FILE *file = fopen(USER_DATA_FILE, "r");
     char stored_username[MAX_LENGTH], stored_password[MAX_LENGTH];
     if (!file) return 0;
     while (fscanf(file, "%s %s", stored_username, stored_password) != EOF) {
         if (strcmp(stored_username, username) == 0) {
             fclose(file);
             return strcmp(stored_password, password) == 0; // Return 1 if password matches, otherwise return 0
         }
     }
     fclose(file);
     return 0;
 }

 // Function to receive credentials from the client (for both signup and login)
 void receiveCredentials(int client_socket, User *user, int isSignup) {
     recv(client_socket, user->username, MAX_LENGTH, 0);
     recv(client_socket, user->password, MAX_LENGTH, 0);
 }

 // Function to send a response back to the client
 void sendResponse(int client_socket, const char *response) {
     send(client_socket, response, strlen(response) + 1, 0);
 }

 // Function to lock the mutex
 void lockMutex() {
     pthread_mutex_lock(&mutex);
 }

 // Function to unlock the mutex
 void unlockMutex() {
     pthread_mutex_unlock(&mutex);
 }

 // Placeholder function for processing CDR files
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
             // Prompt the user for MSISDN
             customerBilling(client_socket);
             break;

         case USER_BILLING_ACTION_INTEROPERATOR:
             interoperatorBilling(client_socket);
             break;

         default:
             strcpy(response, "Invalid Billing Choice option.");
     }

     send(client_socket, response, strlen(response) + 1, 0);
 }
void init_customer_data(CustomerData *op_data) {
    op_data->incoming_call_duration_within = 0;
    op_data->outgoing_call_duration_within = 0;
    op_data->incoming_sms_within = 0;
    op_data->outgoing_sms_within = 0;
    op_data->incoming_call_duration_outside = 0;
    op_data->outgoing_call_duration_outside = 0;
    op_data->incoming_sms_outside = 0;
    op_data->outgoing_sms_outside = 0;
    op_data->mb_downloaded = 0;
    op_data->mb_uploaded = 0;
}
void process_customer_by_id(const char *filename, const char *msisdn, CustomerData *data) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("File opening failed");
        return;
    }

    char line[BUFFER_SIZE];
    int flag = 0;
    while (fgets(line, sizeof(line), file)) {
        char *token;
        char first_record_msisdn[20], second_record_msisdn[20], brand[65], first_op_mmc[7] = "", second_op_mmc[7] = "", call_type[8];
        long duration = 0, download = 0, upload = 0;

        // Read and parse the line
        token = strtok(line, "|");
        if (token == NULL) continue;
        strcpy(first_record_msisdn, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(brand, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(first_op_mmc, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(call_type, token);

        token = strtok(NULL, "|");
        duration = atol(token);

        token = strtok(NULL, "|");
        download = atol(token);

        token = strtok(NULL, "|");
        upload = atol(token);
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(second_record_msisdn, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(second_op_mmc, token);

        if (strcmp(first_record_msisdn, msisdn) == 0) {
            strcpy(data->msisdn, first_record_msisdn);
            strcpy(data->brand, brand);
            strcpy(data->mmc, first_op_mmc);

            // Update the counts based on the call type
            if (strcmp(call_type, "MTC") == 0) {
                if(strcmp(first_op_mmc, second_op_mmc) == 0){
                data->incoming_call_duration_within += duration;
                }
                else{
                    data->incoming_call_duration_outside += duration;
                }
            } else if (strcmp(call_type, "MOC") == 0) {
                if(strcmp(first_op_mmc, second_op_mmc) == 0){
                    data->outgoing_call_duration_within += duration;
                }
                else{
                    data->outgoing_call_duration_outside += duration;
                }
            } else if (strcmp(call_type, "SMS-MO") == 0) {
                if(strcmp(first_op_mmc, second_op_mmc) == 0){
                    data->outgoing_sms_within ++;
                }
                else{
                    data->outgoing_sms_outside ++;
                }
            } else if (strcmp(call_type, "SMS-MT") == 0) {
                if(strcmp(first_op_mmc, second_op_mmc) == 0){
                    data->incoming_sms_within ++;
                }
                else{
                    data->incoming_sms_outside ++;
                }
            } else if (strcmp(call_type, "GPRS") == 0) {
                data->mb_downloaded += download;
                data->mb_uploaded += upload;
            }
        }
    }
    fclose(file);
}
int find_or_create_customer(CustomerSummary **customers, int *customer_count, const char *brand, const char *id) {
    for (int i = 0; i < *customer_count; i++) {
        if (strcmp(customers[i]->brand, brand) == 0 && strcmp(customers[i]->id, id) == 0) {
            return i;
        }
    }
    // Add new operator if not found
    customers[*customer_count] = (CustomerSummary* )malloc(sizeof(CustomerSummary)); // Allocate memory for a new operator
    if (customers[*customer_count] == NULL) {
        perror("Failed to allocate memory for customer");
        exit(EXIT_FAILURE);
    }
    strcpy(customers[*customer_count]->brand, brand);
    strcpy(customers[*customer_count]->id, id);
    customers[*customer_count]->data = (CustomerData){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Initialize data
    (*customer_count)++;
    return *customer_count - 1;
}

void process_customer_whole(const char *filename, CustomerSummary **customers) {
    int customer_count = 0;
    FILE *file = fopen(filename, "r");
    FILE *output_file = fopen("IOSB.txt", "w");
    if (!file) {
        perror("Input File opening failed");
        return;
    }
    if (!output_file) {
        perror("Output File opening failed");
        return;
    }
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        char *token;
        char first_record_msisdn[20], second_record_msisdn[20], brand[65], first_op_mmc[7] = "", second_op_mmc[7] = "", call_type[8];
        long duration = 0, download = 0, upload = 0;

        token = strtok(line, "|");
        if (token == NULL) continue;
        strcpy(first_record_msisdn, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(brand, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(first_op_mmc, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(call_type, token);

        token = strtok(NULL, "|");
        duration = atol(token);

        token = strtok(NULL, "|");
        download = atol(token);

        token = strtok(NULL, "|");
        upload = atol(token);
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(second_record_msisdn, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(second_op_mmc, token);

        int customer_index = find_or_create_customer(customers, &customer_count, brand, first_record_msisdn);
        strcpy(customers[customer_index]->data.msisdn, first_record_msisdn);
        strcpy(customers[customer_index]->data.brand, brand);
        strcpy(customers[customer_index]->data.mmc, first_op_mmc);

        // Update the counts based on the call type
        if (strcmp(call_type, "MTC") == 0) {
            if(strcmp(first_op_mmc, second_op_mmc) == 0){
            customers[customer_index]->data.incoming_call_duration_within += duration;
            }
            else{
                customers[customer_index]->data.incoming_call_duration_outside += duration;
            }
        } else if (strcmp(call_type, "MOC") == 0) {
            if(strcmp(first_op_mmc, second_op_mmc) == 0){
                customers[customer_index]->data.outgoing_call_duration_within += duration;
            }
            else{
                customers[customer_index]->data.outgoing_call_duration_outside += duration;
            }
        } else if (strcmp(call_type, "SMS-MO") == 0) {
            if(strcmp(first_op_mmc, second_op_mmc) == 0){
                customers[customer_index]->data.outgoing_sms_within ++;
            }
            else{
                customers[customer_index]->data.outgoing_sms_outside ++;
            }
        } else if (strcmp(call_type, "SMS-MT") == 0) {
            if(strcmp(first_op_mmc, second_op_mmc) == 0){
                customers[customer_index]->data.incoming_sms_within ++;
            }
            else{
                customers[customer_index]->data.incoming_sms_outside ++;
            }
        } else if (strcmp(call_type, "GPRS") == 0) {
            customers[customer_index]->data.mb_downloaded += download;
            customers[customer_index]->data.mb_uploaded += upload;
        }
    }
    
    for (int i = 0; i < customer_count; i++) {
        fprintf(output_file,
            "Customer ID: %s (%s)\n"
            "\t* Services within the mobile operator *\n"
            "\tIncoming voice call durations: %ld\n"
            "\tOutgoing voice call durations: %ld\n"
            "\tIncoming SMS messages: %ld\n"
            "\tOutgoing SMS messages: %ld\n"
            "\t* Services outside the mobile operator *\n"
            "\tIncoming voice call durations: %ld\n"
            "\tOutgoing voice call durations: %ld\n"
            "\tIncoming SMS messages: %ld\n"
            "\tOutgoing SMS messages: %ld\n"
            "\t* Internet use *\n"
            "\tMB downloaded: %ld | MB uploaded: %ld\n\n",
            customers[i]->id, customers[i]->brand,
            customers[i]->data.incoming_call_duration_within,
            customers[i]->data.outgoing_call_duration_within,
            customers[i]->data.incoming_sms_within,
            customers[i]->data.outgoing_sms_within,
            customers[i]->data.incoming_call_duration_outside,
            customers[i]->data.outgoing_call_duration_outside,
            customers[i]->data.incoming_sms_outside,
            customers[i]->data.outgoing_sms_outside,
            customers[i]->data.mb_downloaded,
            customers[i]->data.mb_uploaded);
    }
    fclose(file);
    fclose(output_file);
    printf("Data processing complete. Output saved to processed_CDR_customers.txt.txt\n");
}

// Function for customer billing
void customerBilling(int client_socket) {
    int billingChoice;
    char response[BUFFER_SIZE];
    CustomerData data;
	CustomerSummary *customers[ENTRIES]; //
    char msisdn[MAX_LENGTH];
    //sendResponse(client_socket, "Customer Billing Menu:\n1. Search for MSISDN\n2. Get all processed data (CB.txt)");
    recv(client_socket, &billingChoice, sizeof(billingChoice), 0);

    switch (billingChoice) {
        case USER_BILLING_ACTION_MSISDN:
            recv(client_socket, msisdn, MAX_LENGTH, 0); // Receive MSISDN from client
            // Echo back the entered MSISN
            init_customer_data(&data);
            process_customer_by_id("user.txt", msisdn, &data); // Process the CDRs for the given MSISDN
            sprintf(response,
        "#Customer Data Base:\n"
        "Customer ID: %s (%s)\n"
        "\t* Services within the mobile operator *\n"
        "\tIncoming voice call durations: %ld\n"
        "\tOutgoing voice call durations: %ld\n"
        "\tIncoming SMS messages: %ld\n"
        "\tOutgoing SMS messages: %ld\n"
        "\t* Services outside the mobile operator *\n"
        "\tIncoming voice call durations: %ld\n"
        "\tOutgoing voice call durations: %ld\n"
        "\tIncoming SMS messages: %ld\n"
        "\tOutgoing SMS messages: %ld\n"
        "\t* Internet use *\n"
        "\tMB downloaded: %ld | MB uploaded: %ld\n",
        data.msisdn, data.brand,
        data.incoming_call_duration_within,
        data.outgoing_call_duration_within,
        data.incoming_sms_within,
        data.outgoing_sms_within,
        data.incoming_call_duration_outside,
        data.outgoing_call_duration_outside,
        data.incoming_sms_outside,
        data.outgoing_sms_outside,
        data.mb_downloaded,
        data.mb_uploaded);
        break;

        case USER_BILLING_ACTION_CSFILE:
            process_customer_whole("user.txt", customers);
            strcpy(response, "Data saved to CB.txt file.");
            break;

        default:
            strcpy(response, "Invalid Customer Billing option.");
    }
    send(client_socket, response, strlen(response) + 1, 0);
}

// Function to initialize operator data
void init_operator_data(OperatorData *op_data) {
    op_data->incoming_call_duration = 0;
    op_data->outgoing_call_duration = 0;
    op_data->incoming_sms = 0;
    op_data->outgoing_sms = 0;
    op_data->mb_downloaded = 0;
    op_data->mb_uploaded = 0;
}

// Function to process the CDR file
void process_interoperator_by_id(const char *filename, const char *mmc, OperatorData *data) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("File opening failed");
        return;
    }
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        char *token;
        char msisdn[20], brand[65], op_mmc[7], call_type[8];
        long duration = 0, download = 0, upload = 0;

        token = strtok(line, "|");
        strcpy(msisdn, token);
        token = strtok(NULL, "|");
        strcpy(brand, token);
        token = strtok(NULL, "|");
        strcpy(op_mmc, token);
        token = strtok(NULL, "|");
        strcpy(call_type, token);
        token = strtok(NULL, "|");
        duration = atol(token);
        token = strtok(NULL, "|");
        download = atol(token);
        token = strtok(NULL, "|");
        upload = atol(token);

        // Check if the current record matches the requested MMC/MNC
        if (strcmp(op_mmc, mmc) == 0) {
            strcpy(data->brand, brand);
            strcpy(data->mmc, op_mmc);
            // Update the counts based on the call type
            if (strcmp(call_type, "MTC") == 0) {
                data->incoming_call_duration += duration;
            } else if (strcmp(call_type, "MOC") == 0) {
                data->outgoing_call_duration += duration;
            } else if (strcmp(call_type, "SMS-MO") == 0) {
                data->outgoing_sms++;
            } else if (strcmp(call_type, "SMS-MT") == 0) {
                data->incoming_sms++;
            } else if (strcmp(call_type, "GPRS") == 0) {
                data->mb_downloaded += download;
                data->mb_uploaded += upload;
            }
        }
    }
    fclose(file);
}
int find_or_create_operator(OperatorSummary **operators, int *operator_count, const char *brand, const char *id) {
    for (int i = 0; i < *operator_count; i++) {
        if (strcmp(operators[i]->brand, brand) == 0 && strcmp(operators[i]->id, id) == 0) {
            return i;
        }
    }
    // Add new operator if not found
    operators[*operator_count] = (OperatorSummary* )malloc(sizeof(OperatorSummary)); // Allocate memory for a new operator
    if (operators[*operator_count] == NULL) {
        perror("Failed to allocate memory for operator");
        exit(EXIT_FAILURE);
    }
    strcpy(operators[*operator_count]->brand, brand);
    strcpy(operators[*operator_count]->id, id);
    operators[*operator_count]->data = (OperatorData){0, 0, 0, 0, 0, 0}; // Initialize data
    (*operator_count)++;
    return *operator_count - 1;
}

void process_interoperator_whole(const char *filename, OperatorSummary **operators) {
    int operator_count = 0;
    FILE *file = fopen(filename, "r");
    FILE *output_file = fopen("processed_CDR_operators.txt", "w");
    if (!file) {
        perror("Input File opening failed");
        return;
    }
    if (!output_file) {
        perror("Output File opening failed");
        return;
    }
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        char *token;
        char msisdn[20], brand[65], op_mmc[7], call_type[8];
        long duration = 0, download = 0, upload = 0;

        token = strtok(line, "|");
        strcpy(msisdn, token);
        token = strtok(NULL, "|");
        strcpy(brand, token);
        token = strtok(NULL, "|");
        strcpy(op_mmc, token);
        token = strtok(NULL, "|");
        strcpy(call_type, token);
        token = strtok(NULL, "|");
        duration = atol(token);
        token = strtok(NULL, "|");
        download = atol(token);
        token = strtok(NULL, "|");
        upload = atol(token);

        int operator_index = find_or_create_operator(operators, &operator_count, brand, op_mmc);
        strcpy(operators[operator_index]->data.brand, brand);
        strcpy(operators[operator_index]->data.mmc, op_mmc);
        if (strcmp(call_type, "MTC") == 0) {
            operators[operator_index]->data.incoming_call_duration += duration;
        } else if (strcmp(call_type, "MOC") == 0) {
            operators[operator_index]->data.outgoing_call_duration += duration;
        } else if (strcmp(call_type, "SMS-MO") == 0) {
            operators[operator_index]->data.outgoing_sms ++;
        } else if (strcmp(call_type, "SMS-MT") == 0) {
            operators[operator_index]->data.incoming_sms++;
        } else if (strcmp(call_type, "GPRS") == 0) {
            operators[operator_index]->data.mb_downloaded += download;
            operators[operator_index]->data.mb_uploaded += upload;
        }
    }
    for (int i = 0; i < operator_count; i++) {
        fprintf(output_file, "Operator Brand: %s (%s)\n"
                        "\tIncoming voice call durations: %ld\n"
                        "\tOutgoing voice call durations: %ld\n"
                        "\tIncoming SMS messages: %ld\n"
                        "\tOutgoing SMS messages: %ld\n"
                        "\tMB downloaded: %ld | MB uploaded: %ld\n\n",
                operators[i]->brand, operators[i]->id,
                operators[i]->data.incoming_call_duration, operators[i]->data.outgoing_call_duration,
                operators[i]->data.incoming_sms, operators[i]->data.outgoing_sms,
                operators[i]->data.mb_downloaded, operators[i]->data.mb_uploaded);
    }
    fclose(file);
    fclose(output_file);
    printf("Data processing complete. Output saved to IOSB.txt\n");
}

 // Placeholder function for interoperator billing
void interoperatorBilling(int client_socket) {
    int billingChoice;
    char response[BUFFER_SIZE];
    OperatorData data;
    OperatorSummary *operators[100];
    char operatorID[MAX_LENGTH];
    recv(client_socket, &billingChoice, sizeof(billingChoice), 0);
    switch (billingChoice) {
        case USER_BILLING_ACTION_OPID:
            recv(client_socket, operatorID, MAX_LENGTH, 0); // Receive MSISDN from client
            init_operator_data(&data);
            process_interoperator_by_id("user.txt",operatorID,&data);
            sprintf(response, "# Operator Data Base:\nOperator Brand: %s (%s)\n"
                        "\tIncoming voice call durations: %ld\n"
                        "\tOutgoing voice call durations: %ld\n"
                        "\tIncoming SMS messages: %ld\n"
                        "\tOutgoing SMS messages: %ld\n"
                        "\tMB downloaded: %ld | MB uploaded: %ld\n",
                data.brand, data.mmc,
                data.incoming_call_duration,
                data.outgoing_call_duration,
                data.incoming_sms,
                data.outgoing_sms,
                data.mb_downloaded,
                data.mb_uploaded);
            break;

        case USER_BILLING_ACTION_OPFILE:
            process_interoperator_whole("user.txt", operators);
            strcpy(response, "Data saved to IOSB.txt file.");
            break;

        default:
            strcpy(response, "Invalid Interoperator Billing option.");
    }
    send(client_socket, response, strlen(response) + 1, 0);
}

