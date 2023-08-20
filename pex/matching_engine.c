#include "pe_exchange.h"
#include "matching_engine.h"

#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#define MAX_PRICE 10000001
#define MIN_PRICE -1
#define INITIAL_OVERLAPS 10
/*
Use two ordered priority queues implemented as linked lists. 
-> Implementing linked list functionality first 
Buy orders - sorted max->min with price
Sell orders - sorted min->max with price 
*/

DESC * new_desc(int trader_id, int order_id, unsigned long tp) // ! EDITED 4/5
{
    DESC * head = malloc(sizeof(DESC));
    head->trader_id = trader_id;
    head->order_id = order_id;
    head->next = NULL; 
    head->quantity = -1;
    head->time_priority = tp;
    return head; 
}

NODE * new_node(int price, int num, int level, DESC* d) // ! EDITED 4/5
{
    NODE * head = malloc(sizeof(NODE));
    head->price = price; 
    head->num = num;
    head->level = level;
    head->adjacent = d; // No bids for the head node 
    head->next = NULL;  // Points to null currently 
    return head;
}

ITEM * new_item(int buy_size, int sell_size)
{
    /*
    :: Node * queues[2] :: 
    This parameter is an array that holds pointers to the starting nodes
    of the linked list. 

    This function initiates the orderset for a new item

    Two linked lists will be referenced to by an array: 
    [buy_orders, sell_orders]

    */
    ITEM * queues = malloc(sizeof(ITEM));
    queues->buy_size = buy_size; 
    queues->sell_size = sell_size; 
    DESC * q_b_desc = new_desc(-1, -1, 0);
    DESC * q_s_desc = new_desc(-1, -1, 0);
    queues->buys = new_node(MAX_PRICE, -1, -1, q_b_desc);
    queues->sells = new_node(MIN_PRICE, -1, -1, q_s_desc);
    return queues;
}

DESC * traverse_descs(NODE * d)
{
    /*
    ? Traverses to the end of a DESC linked list
    */
   if(d == NULL)
   {
        return NULL;
   }

   DESC * c = d->adjacent;
   while(c->next != NULL)
   {
        c = c->next;
   }
   return c;
}

NODE * traverse_nodes(NODE* n, int price_to_find)
{
    /*
    ? Finds a price in a linked list 
    */
    NODE * c = n;
    while(c != NULL)
    {
        if(c->price == MAX_PRICE || c->price == MIN_PRICE)
        {
            c = c->next;
            continue;
        }
        if(c->price == price_to_find)
        {
            return c;
        }
        c = c->next;
    }
    return NULL;
}

int greater(int a, int b)
{
    return a > b;
}

int lesser(int a, int b)
{
    return a < b;
}

void insert_sorted(NODE ** head, struct order * o, DESC* n_d, 
int current_price, int (*comparator)(int, int))
{
    /*
    ? Node must be a new price. Hence, we make a new one. 
    */
    if(*head == NULL)
    {
        return;
    }
    NODE* temp_node = new_node(current_price, o->qty, 1, n_d);
    
    NODE * cursor = (*head)->next; 
    NODE * prev = *head;
    while(cursor != NULL && comparator(cursor->price, current_price)) 
    {
        prev = cursor; 
        cursor = cursor->next; 
    }

    if(prev == *head)
    {
        temp_node->next = cursor; 
        prev->next = temp_node; 
    }
    else
    {
        temp_node->next = cursor; 
        prev->next = temp_node; 
    }

}

void add_buy(ITEM * queues, struct order * o, int trader_index, unsigned long tp)
{
    // Get the head node of the queue
    // This is a pointer to the first actual node in the LL 
    NODE * buy_head = queues->buys;
    
    // This holds the order information 
    DESC * n_d = new_desc(trader_index, o->order_id, tp);

    // c_price stands for current price (of the new order). 
    // This is used to sort 
    int c_price = o->price;

    // Newest quantity added
    n_d->quantity = o->qty;

    // If an order is added as a descriptor, we only need to increment level
    // Otherwise, we need to increment the whole buy_size
    int added_as_descriptor = 0;

    // ? Assesses to see if the node already exists
    NODE * node_exists = traverse_nodes(buy_head, c_price);

    // ? If the price is new, we need to add both a node and descriptor
    if(node_exists != NULL && node_exists->price != MAX_PRICE)
    {
        /*
        ? Price exists, add it as a desc to the end of an existing node
        ? Increment the node's level as increased by 1
        ? Mark operation as "added_as_descriptor"
        */
        added_as_descriptor = 1; // Buy_size needs to increase. 
        // ? Adds desc to the end of the existing descendants. 
        // ? Thus it has the lowest time priority. 
        DESC * end = traverse_descs(node_exists);
        end->next = n_d;
        node_exists->level += 1;
        node_exists->num += o->qty;
        return;
    }

    insert_sorted(&buy_head, o, n_d, c_price, greater); // ! PERHAPS ERROR?

    if(!added_as_descriptor)
    {
        queues->buy_size += 1;
    }
}

void add_sell(ITEM * queues, struct order * o, int trader_index, unsigned long tp)
{
    NODE * sell_head = queues->sells; 
    DESC * n_d = new_desc(trader_index, o->order_id, tp);
    n_d->quantity = o->qty;
    int c_price = o->price; 
    
    int added_as_descriptor = 0;

    NODE * node_exists = traverse_nodes(sell_head, c_price);

    if(node_exists != NULL && node_exists->price != MIN_PRICE)
    {
        added_as_descriptor = 1; 
        DESC * end = traverse_descs(node_exists);
        end->next = n_d;
        node_exists->level += 1;
        node_exists->num += o->qty;
        return;
    }

    insert_sorted(&sell_head, o, n_d, c_price, lesser);

    if(!added_as_descriptor)
    {
        queues->sell_size += 1;
    }
}

void print_buy_nodes(NODE* h)
{
    NODE * c = h;
    // int i = 0;
    // printf("Starting node outside: %d\n", c->price);
    
    while(c->next != NULL)
    {
        //printf("C[%d]: %d\n", i++,c->price);
        // printf("Starting node: %d\n", c->price);
        if(c->adjacent == NULL || c->level == -1)
        {
            c = c->next;
            continue;
        }
        
        if(c->level == 1)
        {
            printf("%s\t\tBUY %d @ $%d (%d order)\n", LOG_PREFIX, c->num, c->price, c->level);
        }
        else
        {
            printf("%s\t\tBUY %d @ $%d (%d orders)\n", LOG_PREFIX, c->num, c->price, c->level);
        }
        c = c->next;
    }

    if(c->level < 0)
    {
        c = c->next;
    }
    

    if(c == NULL)
    {
        return;
    }
    // printf("Starting node after 2: %d\n", c->price);
    if(c->level == 1)
    {
        printf("%s\t\tBUY %d @ $%d (%d order)\n", LOG_PREFIX, c->num, c->price, c->level);
    }
    else
    {
        printf("%s\t\tBUY %d @ $%d (%d orders)\n", LOG_PREFIX, c->num, c->price, c->level);
    }
}

void print_sell_nodes(NODE* h)
{
    if(h == NULL)
    {
        return;
    }
    else
    {
        print_sell_nodes(h->next);
        if(h->level < 0)
        {
            return;
        }
        if(h->level == 1)
        {
            printf("%s\t\tSELL %d @ $%d (%d order)\n", LOG_PREFIX, h->num, h->price, h->level);
        }
        else
        {
            printf("%s\t\tSELL %d @ $%d (%d orders)\n", LOG_PREFIX, h->num, h->price, h->level);
        }
    }
}

int len_buy(ITEM* i)
{
    int size = 0;
    NODE * c = i->buys;
    while(c->next != NULL)
    {
        size++;
        c = c->next;
    }
    return size;
}

int len_sell(ITEM* i)
{
    int size = 0;
    NODE * c = i->sells;
    while(c->next != NULL)
    {
        size++;
        c = c->next;
    }
    return size;
}

void remove_first(NODE * n)
{
    // ? A successful removal results in a decrement of level
    if(n == NULL)
    {
        return;
    }

    DESC* temp = n->adjacent; 
    n->adjacent = (temp)->next;
    free(temp);
    return;
}

void delete_node(NODE** head_ptr, NODE* to_remove)
{
    if(head_ptr == NULL || to_remove == NULL)
    {
        return; 
    }

    NODE* head = *head_ptr;
    NODE * c = head; 
    NODE * prev = head;
    if(head->price == MAX_PRICE || head->price == MIN_PRICE)
    {
        c = head->next;
    }
    while(c != NULL)
    {
        if(c == to_remove)
        {
            prev->next = c->next;
            free(c);
            return;
        }
        c = c->next;
        prev = prev->next;
    }
    // Getting to here means c is null. C was not in the list
    prev->next = NULL; // TODO ASK ABOUT IT
    return;
}

void remove_desc(NODE * n, DESC* to_remove)
{
    // ? A successful removal results in a decrement of level
    // Double free? 
    if(n == NULL || to_remove == NULL)
    {
        return;
    }
    
    DESC * cursor = n->adjacent;
    if(cursor == to_remove)
    {
        n->adjacent = cursor->next;
        free(to_remove);
        return;
    }

    DESC * previous = cursor; 

    while(cursor != to_remove)
    {
        previous = cursor; 
        cursor = cursor->next; 
        if(cursor == NULL)
        {
            // Not in linked list
            return;
        }
    }
    DESC * to_free = cursor;
    previous->next = to_free->next; 
    free(to_free);
    return;
}

int64_t round_fee(long double to_round)
{
    // printf("fee to round: %f\n", to_round);
    int64_t integer = (int64_t) to_round; 

    int64_t decimal = integer % 100;
    // printf("Integer part: %d\nDecimal part: %f\n", integer, decimal);

    if(decimal >= 50)
    {
        return integer/100 + 1; 
    }
    return integer/100;
}

int check_active_trader(int* active_traders, int trader_id)
{
    if(active_traders[trader_id])
    {
        return 1; // Active 
    }
    return 0; // Inactive
}

int check_node(NODE* head_ptr, NODE* to_check)
{
    if(to_check->level == -1)
    {
        return 0;
    }
    if(to_check->level == 0 && to_check->num == 0) // || adjacent == NULL
    {
        // TODO: IDENTIFY WHERE ADJACENT ISN"T TURNING NULL:  && to_check->adjacent == NULL
        delete_node(&head_ptr, to_check);
        return 1;
    }
    return 0;
}

DESC * help_find_descriptor(NODE * price_node, 
int trader_id_tf, int order_id_tf)
{
    if(price_node ==NULL)
    {
        return NULL;
    }

    DESC * start_d = price_node->adjacent;

    while(start_d != NULL)
    {
        
        if(start_d->order_id == order_id_tf && start_d->trader_id == trader_id_tf)
        {
            // ? Match found

            return start_d;
        }
        start_d = start_d->next;
    }
    return NULL;
}

DESC * find_descriptor(ITEM * i, int trader_id, int order_id, 
NODE ** ptr_to_node, char * buy_or_sell)
{
    /*
    ? Used for the CANCEL and AMEND commands
    ::RETURNS NULL if descriptor isn't found, RETURNS Pointer to descriptor if it is::
        */
    // ? Check buys then sells 
    DESC * to_return = NULL;
    NODE * buy_cursor = i->buys;
    if(buy_cursor == NULL)
    {
        return NULL;
    }
    while(buy_cursor != NULL)
    {
        if(buy_cursor->price == MAX_PRICE)
        {
            buy_cursor = buy_cursor->next;
            continue;
        }
        to_return = help_find_descriptor(buy_cursor, trader_id, order_id);
        if(to_return != NULL)
        {
            // ? Found the order
            *buy_or_sell = 'b';
            *ptr_to_node = buy_cursor;
            
            return to_return;
        }
        buy_cursor = buy_cursor->next;
    }

    NODE * sell_cursor = i->sells;
    if(sell_cursor == NULL)
    {

        return NULL;
    }
    while(sell_cursor != NULL)
    {
        if(sell_cursor->price == MIN_PRICE)
        {
            sell_cursor = sell_cursor->next;
            continue;
        }
        to_return = help_find_descriptor(sell_cursor, trader_id, order_id);
        if(to_return != NULL)
        {

            *ptr_to_node = sell_cursor;
            *buy_or_sell = 's';

            return to_return;
        }
        sell_cursor = sell_cursor->next;
    }
    return NULL;
}

// TODO: CHECK QUANTITY OPS FOR AMEND AND CANCEL
void ammend_operations(ITEM* i, NODE* to_check, DESC* to_amend, 
int new_price, int new_quantity, char buy_or_sell, unsigned long tp)
{
    struct order * o = malloc(sizeof(struct order));
    // Save desc order information 
    int trader_index = to_amend->trader_id;
    to_check->level -= 1;
    (to_check)->num = (to_check)->num - to_amend->quantity;
    
    o->command_name = "AMEND"; 
    o->order_id = to_amend->order_id;
    o->price = new_price;
    o->product_name = "BRUH";
    o->qty = new_quantity;

    // ? remove descriptor 
    
    remove_desc(to_check, to_amend); //! isnt removing properly
    check_node(i->buys, to_check);
    check_node(i->sells, to_check);

    // ? Check nodes
    if('b' == buy_or_sell)
    {
        //? Buy operation

        
        add_buy(i, o, trader_index, tp);
    }
    else
    {
        //? Sell operation
        add_sell(i, o, trader_index, tp);
    }
    free(o);
}

void cancel_operations(ITEM* i, NODE* to_check, DESC * to_remove, char buy_or_sell)
{
    to_check->level -= 1;
    (to_check)->num = (to_check)->num - to_remove->quantity;
    check_node(i->buys, to_check);
    check_node(i->sells, to_check);

    remove_desc(to_check, to_remove);
}

void message_pipes(int* write_fds, int * trader_pids, int trader_id, char* msg, int order_id, int qty)
{
    char msg_to_write[CMD_SIZE];
    memset(msg_to_write, 0, sizeof(msg_to_write));
    snprintf(msg_to_write, sizeof(msg_to_write), msg, order_id, qty);
    if(write(*(write_fds + trader_id), msg_to_write, strlen(msg_to_write)) == -1)
    {
        perror("ERROR IN WRITING");
        return;
    }
    kill(*(trader_pids + trader_id), SIGUSR1);
}

void update_trader_attributes(TRADER** traders,int buyer_id, 
int seller_id, int64_t items_transacted, int64_t price_diff, 
char most_recent, int item_number, int * active_traders)
{
    // most recent is either 'b' or 's';
    if(most_recent == 'b')
    {
        // Trader taxes buyer 
        (traders[seller_id]->debt)[item_number] += ((int64_t) price_diff);
        (traders[buyer_id]->debt)[item_number] -= (((int64_t)price_diff) + round_fee(price_diff));
    }
    else
    {
        // Trader taxes seller 
        (traders[buyer_id]->debt)[item_number] -= ((int64_t) price_diff);
        (traders[seller_id]->debt)[item_number] += (((int64_t)price_diff) - round_fee(price_diff));

    }

    // Buyer gets debt increased and items increased 
    (traders[buyer_id]->items)[item_number] += items_transacted;
    
    // Seller gets debt decreased and items decreased
    (traders[seller_id]->items)[item_number] -= items_transacted;
}

int64_t update(NODE* buy_node, NODE* sell_node, DESC* buy_order, DESC* sell_order,
int* write_fds, int* trader_pids, int price, int mr_trader_id, int mr_order_id,
TRADER** traders, int item_number, int* active_traders)
{   
    int buy_num = buy_order->quantity;
    int sell_num = sell_order->quantity; 
    
    int successful_sales = buy_num - sell_num; 
    // TODO: Identify most recent order. This is the order that is going to be charged to. 
        // ? mr_* means most recent 
    int64_t fee = 0;

    int buy_most_recent = 0;
    if(buy_order->order_id == mr_order_id && buy_order->trader_id == mr_trader_id)
    {
        // ? Buy was most recent Sell is the older order. 
        buy_most_recent = 1; 

    }
    else
    {
        // ? Sell was most recent Buy is the older order 
        buy_most_recent = 0; 

    }
    // ! Send to buyer first, then seller
    
    if(successful_sales < 0)
    {

        int64_t spent = ((int64_t) price) * ((int64_t) buy_num);
        fee = round_fee((long double)((spent)));

        // ? Negative means more sells than buys 
        // ? By extension this means we use the buys as the minimum quantity
        // ? buy_order->quantity is the quant everything is depleted by 
        // SEND MSG to both pipes 
        if(check_active_trader(active_traders, buy_order->trader_id))
        {
            message_pipes(write_fds, trader_pids, buy_order->trader_id, 
        FILL_MSG, buy_order->order_id, buy_order->quantity);
        }
        
        if(check_active_trader(active_traders, sell_order->trader_id))
        {
           message_pipes(write_fds, trader_pids, sell_order->trader_id, 
        FILL_MSG, sell_order->order_id, buy_order->quantity); 
        }

        // ? Delete the buys, decrement the sells. 
        // ? 
        if(buy_most_recent)
        { // ? Buy was the most recent added, so sell is the older one
        // COMPLEX OFFENDER HERE sell_order->trader_id is errorred. 
            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", 
            LOG_PREFIX, sell_order->order_id, sell_order->trader_id,
            buy_order->order_id, buy_order->trader_id, spent, fee); 
        }

        else
        { // ? Sell was the most recent added, so buy is the older one
        // ! PRoblem one ! ! ! Buy order trader Id is wrong. 
            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", 
        LOG_PREFIX, buy_order->order_id, buy_order->trader_id, 
        sell_order->order_id, sell_order->trader_id, spent, fee); 
        }
        int saved_buy = buy_order->trader_id; 

        

        // Decrement buy level by 1 as the buy adjacent was removed 
        buy_node->next->level-=1;
        
        // Reduce quantity stored in sell node
        sell_order->quantity = sell_num - buy_num;
        buy_order->quantity = sell_num - buy_num;

        // Reduce total orders at price (num)
        (sell_node->next)->num = (sell_node->next)->num - buy_num;
        buy_node->next->num = buy_node->next->num - buy_num;
        // Check if the buy node is empty. Sell node definitely isnt' empty

        // Delete buy order adjacent 
        remove_first(((buy_node->next)));
        check_node(buy_node, buy_node->next);

        if(buy_most_recent)
        {
            update_trader_attributes(traders, saved_buy, sell_order->trader_id, 
            buy_num, (int64_t) spent, 'b', item_number, active_traders);
        }
        else
        {
            update_trader_attributes(traders, saved_buy, sell_order->trader_id, 
            buy_num, (int64_t) spent, 's', item_number, active_traders);
        }

        return fee;
    }
    
    else if(successful_sales == 0)
    {

        int64_t spent =((int64_t) price) *((int64_t) buy_num);

        fee = round_fee((long double)((spent)));
        // Equals means there was enough to fulfill both
        
        // SEND MSG to both pipes
        if(check_active_trader(active_traders, buy_order->trader_id))
        {
            message_pipes(write_fds, trader_pids, buy_order->trader_id, 
        FILL_MSG, buy_order->order_id, buy_order->quantity);
        }
        if(check_active_trader(active_traders, sell_order->trader_id))
        {
            message_pipes(write_fds, trader_pids, sell_order->trader_id, 
        FILL_MSG, sell_order->order_id, sell_order->quantity);
        }

        // Print the trade to stdout
        if(buy_most_recent)
        { // ? Buy was the most recent added so sell is the older one
            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", 
            LOG_PREFIX, sell_order->order_id, sell_order->trader_id,
            buy_order->order_id, buy_order->trader_id, spent, fee); // ! NEED TO EDIT PRICE!!!!
        }
        else
        { // ? Sell was the most recent added
            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", 
            LOG_PREFIX, buy_order->order_id, buy_order->trader_id, 
            sell_order->order_id, sell_order->trader_id, spent, fee); // ! NEED TO EDIT PRICE!!!!
        }
        int saved_buy = buy_order->trader_id;
        int saved_sell = sell_order->trader_id;

        

        // DECREMENT LEVEL 
        buy_node->next->level -= 1;
        sell_node->next->level -=1;

        sell_order->quantity = 0;
        buy_order->quantity = 0;

        // Remove the overlapping number
        (sell_node->next)->num = (sell_node->next)->num - buy_num;
        (buy_node->next)->num = (buy_node->next)->num - buy_num;

        // ? Delete both orders.
        remove_first(((buy_node->next)));
        remove_first(((sell_node->next)));
        
        check_node(buy_node, buy_node->next);
        check_node(sell_node, sell_node->next);

        if(buy_most_recent)
        {
            update_trader_attributes(traders, saved_buy, saved_sell, 
            buy_num, (int64_t) spent, 'b', item_number, active_traders);
        }
        else
        {
            update_trader_attributes(traders, saved_buy, saved_sell, 
            buy_num, (int64_t) spent, 's', item_number, active_traders);
        }
        return fee;
    }
    
    else
    {
        // Greater than means more buys than sells
        int64_t spent = ((int64_t) price) * ((int64_t) sell_num);

        fee = round_fee((long double)((spent)));
        // ? Delete the sells, keep the buys 

        // SEND MSG to both pipes
        
        if(check_active_trader(active_traders, buy_order->trader_id))
        {
            message_pipes(write_fds, trader_pids, buy_order->trader_id, 
        FILL_MSG, buy_order->order_id, sell_order->quantity);
        }
        if(check_active_trader(active_traders, sell_order->trader_id))
        {
            message_pipes(write_fds, trader_pids, sell_order->trader_id, 
        FILL_MSG, sell_order->order_id, sell_order->quantity);
        }

        if(buy_most_recent)
        { // ? Buy was the most recent added
            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", 
        LOG_PREFIX, sell_order->order_id, sell_order->trader_id,
        buy_order->order_id, buy_order->trader_id, spent, fee); 
        }
        else
        { // ? Sell was the most recent added
            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", 
        LOG_PREFIX, buy_order->order_id, buy_order->trader_id, 
        sell_order->order_id, sell_order->trader_id, spent, fee); 
        }

        sell_order->quantity = 0; 
        int saved_sell = sell_order->trader_id;
        sell_node->next->level-=1;
        
        buy_order->quantity = buy_num - sell_num;
        
        (sell_node->next)->num = (sell_node->next)->num - sell_num;
        (buy_node->next)->num = (buy_node->next)->num - sell_num;

        remove_first(((sell_node->next)));
        check_node(sell_node, sell_node->next);
        
        if(buy_most_recent)
        {
            update_trader_attributes(traders, buy_order->trader_id, 
            saved_sell, (int64_t) sell_num, spent, 'b', item_number, active_traders);
        }
        else
        {
            update_trader_attributes(traders, buy_order->trader_id, 
            saved_sell, (int64_t) sell_num, spent, 's', item_number, active_traders);
        }
        return fee;
    }
}

int64_t match_orders(ITEM * i, int * write_fds, int * trader_pids,
 int mr_trader_id, int mr_order_id, TRADER** traders, 
 int item_number, int * active_traders)
{
    int64_t fee = 0;

    // TODO: HAVE THE FUNCTION RETURN THE FEE
    NODE* highest_buy = i->buys->next;
    NODE* lowest_sell = i->sells->next;
    while(highest_buy != NULL && lowest_sell != NULL)
    {
        // Get highest buy and lowest sell orders 
        highest_buy = i->buys->next;
        lowest_sell = i->sells->next;
        
        if(highest_buy == NULL || lowest_sell == NULL)
        {
            // If either of them are null, we know we break
            return fee; 
        }

        if(highest_buy->price == MAX_PRICE || lowest_sell->price == MIN_PRICE)
        {
            return fee;
        }
        
        if(highest_buy->price >= lowest_sell->price)
        {
            // A successful match 
            /*
            TODO Idea: The price it's referencing is the head node, which is why it nulls out
            ? get the number of buy orders 
            ? get the number of sell orders 
            */
            
            DESC* buy_order = highest_buy->adjacent;
            DESC* sell_order = lowest_sell->adjacent;

            // ! Check if trader is dc'd 

            int older_order_price;
            if(buy_order->order_id == mr_order_id && buy_order->trader_id == mr_trader_id)
            {
                older_order_price = lowest_sell->price;
            }
            else
            {
                older_order_price = highest_buy->price;
            }
            fee += update(i->buys, i->sells, buy_order, sell_order, 
            write_fds, trader_pids, older_order_price, mr_trader_id, mr_order_id,
            traders, item_number, active_traders);
            // printf("Match made\n");
        }
        else
        {
            // printf("Breaking in while\n");
            break; // No trades occurred
        }
    }
    return fee;
}

void free_adj(NODE * n)
{
    if (n == NULL)
    {
        return; 
    }
    if(n->adjacent == NULL)
    {
        return;
    }
    DESC * cursor = n->adjacent;
    while(cursor != NULL)
    {
        DESC * next = cursor->next;
        free(cursor);
        cursor = next; 
    }
    n->adjacent = NULL;
}

void free_queues(NODE ** head_ptr)
{
    if(head_ptr == NULL)
    {
        return; 
    }
    NODE * head = *head_ptr; 
    if(head == NULL)
    {
        return; 
    }
    NODE * cursor = head; 
    while(cursor != NULL)
    {
        NODE * next = cursor->next;
        if(cursor->adjacent != NULL)
        {
            free_adj((cursor));
        }
        free(cursor);
        cursor = next;
    }
    *head_ptr = NULL; 
}

void free_items(ITEM* i)
{
    free_queues(&(i->buys));
    free_queues(&(i->sells));
    free(i);
}

void print_trader_items(TRADER* t, int n_items, char** items)
{
    for(int i = 0; i < n_items; i++)
    {
        printf("%s", items[i]);
        printf(" %ld", (t->items)[i]);
        printf(" ($%ld)", (t->debt)[i]);
        if(i != n_items - 1)
        {
            printf(", ");
        }
    }
}