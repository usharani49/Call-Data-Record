#ifndef BILLING_H
#define BILLING_H

#include "server.h"

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

typedef struct {
    char brand[50];
    char id[10];
    OperatorData data;
} OperatorSummary;

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

void init_customer_data(CustomerData *op_data);
void init_operator_data(OperatorData *op_data);
void process_customer_whole(const char *filename, CustomerSummary **customers);
void process_interoperator_whole(const char *filename, OperatorSummary **operators);
void process_customer_by_id(const char *filename, const char *msisdn, CustomerData *data);
void process_interoperator_by_id(const char *filename, const char *mmc, OperatorData *data);
int find_or_create_customer(CustomerSummary **customers, int *customer_count, const char *brand, const char *id);
int find_or_create_operator(OperatorSummary **operators, int *operator_count, const char *brand, const char *id);

#endif // BILLING_H
