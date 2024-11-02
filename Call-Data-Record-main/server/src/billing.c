#include "billing.h"
#include "server.h"

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
    // Add new customer if not found
    customers[*customer_count] = (CustomerSummary* )malloc(sizeof(CustomerSummary)); // Allocate memory for a new customer
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
    FILE *output_file = fopen(CUSTOMER_CDR_DATA_FILE, "w");
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
    printf("Data processing complete. Output saved to CB.txt.txt\n");
}

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

   // FILE *file = fopen("../data/user.txt", "r");
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
    FILE *output_file = fopen(OPERATOR_CDR_DATA_FILE, "w");
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
    printf("Data processing complete. Output saved to IOSB.txt.txt\n");
}
