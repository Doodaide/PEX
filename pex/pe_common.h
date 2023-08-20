#ifndef PE_COMMON_H
#define PE_COMMON_H

#define _POSIX_C_SOURCE 199506L

#define CMD_SIZE 1064

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h> 
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

#define FIFO_EXCHANGE "/tmp/pe_exchange_%d"
#define FIFO_TRADER "/tmp/pe_trader_%d"
#define FEE_PERCENTAGE 1

void signal_handler(int signal_number);

int index_of(int * arr, int len, int to_find);

int array_and(int * arr, int len);

struct order * get_command(char * command);

int find_item(char** arr, int len, char* item);

int check_msg(char* msg, char a);

int is_empty(char* str);

int check_if_integer(char * str);

int valid_command(char * str);

int valid_product_name(char * str);

int valid_int(int a);

int valid_str(char * str);

struct order
{
    char * command_name;
    int order_id;
    char * product_name;
    int qty; 
    int price;  

};

#ifdef DEBUG
#define DBG 1
#define DBG_PRINT(stuff, args...) printf(fmt, ##args)
#else 
#define DBG 0
#define DBG_PRINT(stuff, args...) do {} while (0);
#endif

#endif
