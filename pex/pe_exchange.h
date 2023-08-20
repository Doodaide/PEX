#ifndef PE_EXCHANGE_H
#define PE_EXCHANGE_H

#include "pe_common.h"

// Exchange -> trader
#define CMD_SIZE 1064
#define LOG_PREFIX "[PEX]"
#define ACCEPTED_MSG "ACCEPTED %d;"
#define AMENDED_MSG "AMENDED %d;"
#define CANCELLED_MSG "CANCELLED %d;"
#define INVALID_MSG "INVALID;" 

// Exchange -> other traders

#define MARKET_OPEN "MARKET OPEN;"
#define MARKET_MSG "MARKET %s %s %d %d;"
#define FILL_MSG "FILL %d %d;"

typedef struct descriptor
{
    int trader_id; // * Id of trader 
    int order_id; // * Id of order 
    int quantity; // * How much of it 
    struct descriptor* next; // * ptr to next node 
    unsigned long time_priority; 
} DESC;

typedef struct node
{
    int price; // * price bid 
    int num; // * total quantity of orders at that price 
    struct node* next; // * ptr to next price 
    struct descriptor* adjacent; // * ptr to adjacent orders of same price 
    int level; // * How many ORDERS of the same price there are
} NODE;



typedef struct item
{
    int is_empty;
    struct node * buys;
    struct node * sells;
    int buy_size; 
    int sell_size;
} ITEM;

typedef struct trader
{
    int64_t * items;
    int64_t * debt;
} TRADER;

typedef struct pending_events
{
    struct epoll_event value; 
    struct pending_events * next; 
    pid_t trader_id;
    unsigned long time_priority;
} PENDING;

typedef struct return_values
{
    DESC * descriptor_id;
    NODE * parent_node; 
    char buy_or_sell; 
} LOCATION_INFO; 

DESC * new_desc(int trader_id, int order_id, unsigned long time_priority);

NODE * new_node(int price, int num, int level, DESC* d);

ITEM * new_item(int buy_size, int sell_size);

void add_buy(ITEM * queues, struct order * o, int trader_index, unsigned long tp);

void add_sell(ITEM * queues, struct order * o, int trader_index, unsigned long tp);

int len_buy(ITEM* i);

int len_sell(ITEM* i);

void free_items(ITEM* i);

void print_buy_nodes(NODE* h);

void print_sell_nodes(NODE* h);

int64_t match_orders(ITEM * i, int * write_fds, int * trader_pids,
 int mr_trader_id, int mr_order_id, TRADER** traders, 
 int item_number, int * active_traders);

void print_trader_items(TRADER* t, int n_items, char** items);

DESC * find_descriptor(ITEM * i, int trader_id, int order_id, 
NODE ** ptr_to_node, char * buy_or_sell);

void ammend_operations(ITEM* i, NODE* to_check, DESC* to_amend, 
int new_price, int new_quantity, char buy_or_sell, unsigned long tp);

void cancel_operations(ITEM* i, NODE* to_check, DESC * to_remove, char buy_or_sell);

// ! =====================================


#endif
