#include "pe_trader.h"

int index_of(int * arr, int len, int to_find)
{
    for (int i = 0; i < len; i++)
    {
        if(arr[i] == to_find)
        {
            return i;
        }
    }
    return -1;
}

int find_item(char** arr, int len, char* item)
{
    for(int i = 0; i < len; i++)
    {
        if(!strcmp(*(arr + i), item))
        {
            return i;
        }
    }
    return -1;
}

int array_and(int * arr, int len)
{
    int res = 0;
    for(int i = 0; i < len; i++)
    {
        res = res | arr[i]; 
    }
    return res;
}

int check_msg(char * msg, char a)
{
    /*
    ? Every message should end on the semicolon
    */
    int count = 0;
    char *exists = msg;
    while((exists = strchr(exists, a)) != NULL)
    {
        count++; 
        exists++; 
    }

    return count;
}

int check_if_integer(char * str)
{
    if(str == NULL)
    {
        return 0;
    }
    char * end; 
    errno = 0; 
    strtol(str, &end, 10);
    if(errno != 0 || *end != '\0' || end == NULL || str == end)
    {
        return 0;
    }
    return 1;
}

int is_empty(char* str)
{
    if(str[0] == '\0')
    {
        return 1;
    }
    return 0;
}

int valid_command(char * str)
{
    if(str == NULL || is_empty(str))
    {
        return 0;
    }
    char *commands[] = {"BUY", "SELL", "AMEND", "CANCEL"};
    for(int i = 0; i < 4; i++)
    {
        if(!strcmp(commands[i], str))
        {
            return 1;
        }
    }
    return 0;
}

int valid_product_name(char * str)
{
    if(str == NULL || strlen(str) > 16)
    {
        return 0;
    }
    return 1;    
}

int valid_str(char * str)
{
    if(str == NULL || strlen(str) > 16 || is_empty(str))
    {
        return 0;
    }
    return 1;  
}

int valid_int(int a)
{
    if(a < 0 || a > 999999)
    {
        return 0;
    }
    return 1;
}

struct order * get_command(char * command)
{
    // ? Gets command from trader -> exchange and exchange -> trader
    struct order * order_structure = malloc(sizeof(struct order));
    char* cmd; 
    order_structure-> command_name = NULL; 
    order_structure-> order_id = -1;
    order_structure-> product_name = NULL; 
    order_structure-> qty = -1; 
    order_structure-> price = -1; 


    char strTest[1064] = {0}; // ! REMOVE MAGIC NUMBER
    strncpy(strTest, command, 1064); // ! EDIT LATER

    if(!strcmp("MARKET", strtok(strTest, " ")))
    {
        strtok(command, " ");
        cmd = strtok(NULL, " ");
    }
    else
    {
        cmd = strtok(command, " ");
    }
    order_structure ->command_name = cmd;
    char* prod_name;
    int qty; 
    int price;
    /*
    Bruh
    */

    if (!strcmp(cmd, "BUY"))
    {
        // BUY operation
        prod_name = strtok(NULL, " ");
        qty = atoi(strtok(NULL, " "));
        price = atoi(strtok(NULL, ";"));
        // printf("CMD: %s\nOrderId: %d\nName: %s\nqty: %d\nprice: %d\n", cmd, order_id, prod_name, qty, price);
        order_structure-> product_name = prod_name;
        order_structure-> qty = qty;
        order_structure->price = price; 
    }
    else if(!strcmp(cmd, "SELL"))
    {
        // SEll operation
        prod_name = strtok(NULL, " ");
        qty = atoi(strtok(NULL, " "));
        price = atoi(strtok(NULL, ";"));
        // printf("CMD: %s\nOrderId: %d\nName: %s\nqty: %d\nprice: %d\n", cmd, order_id, prod_name, qty, price);
        order_structure-> product_name = prod_name;
        order_structure-> qty = qty;
        order_structure->price = price; 
    }
    else if(!strcmp(cmd, "AMENDED"))
    {
        // AMEND operation 
        qty = atoi(strtok(NULL, " "));
        price = atoi(strtok(NULL, ";"));
        // printf("CMD: %s\nOrderId: %d\nqty: %d\nprice: %d\n", cmd, order_id, qty, price);
        order_structure-> qty = qty;
        order_structure->price = price; 
    }
    else if(!strcmp(cmd, "CANCELLED"))
    {
        // CANCEL operation 
        // printf("CMD: %s\nOrderId: %d\n", cmd, order_id);
        free(order_structure);
        return NULL;
    }


    return order_structure;
}