#include "user_management.h"
#include "billing.h"

void customerBilling(int client_socket) {
    int billingChoice;
    char response[BUFFER_SIZE];
    CustomerData data;
    CustomerSummary *customers[LARGE_BUFFER_SIZE];
    char msisdn[MAX_LENGTH];
    recv(client_socket, &billingChoice, sizeof(billingChoice), 0);

    switch (billingChoice) {
        case 9:
            recv(client_socket, msisdn, MAX_LENGTH, 0);
            init_customer_data(&data);
            process_customer_by_id("user.txt", msisdn, &data);
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
		case 10:
            process_customer_whole("user.txt", customers);
            strcpy(response, "Data saved to CB.txt file.");
            break;

        default:
            strcpy(response, "Invalid Customer Billing option.");
    }
    send(client_socket, response, strlen(response) + 1, 0);
}

void interoperatorBilling(int client_socket) {
    int billingChoice;
    char response[BUFFER_SIZE];
    OperatorData data;
    OperatorSummary *operators[100];
    char operatorID[MAX_LENGTH];
    recv(client_socket, &billingChoice, sizeof(billingChoice), 0);
    switch (billingChoice) {
        case 11:
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

        case 12:
            process_interoperator_whole("user.txt", operators);
            strcpy(response, "Data saved to IOSB.txt file.");
            break;

        default:
            strcpy(response, "Invalid Interoperator Billing option.");
    }
    send(client_socket, response, strlen(response) + 1, 0);
}
