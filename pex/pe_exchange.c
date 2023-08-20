/**
 * comp2017 - assignment 3
 * Alexander He
 * alhe6638
 */

#include "pe_exchange.h"
#define MAX_EVENTS 50

#define LINE_SIZE 256
PENDING * pending_head = NULL; 


// volatile int sigusr1_sent = 0;

void add_pending_event(struct epoll_event * new_event, pid_t t_pid, unsigned long tp)
{
    PENDING * temp = malloc(sizeof(PENDING));
    memcpy(&(temp->value), new_event, sizeof(struct epoll_event));
    temp->trader_id = t_pid;
    temp ->next = pending_head;
    temp->time_priority = tp;
    pending_head = temp;
}

PENDING * remove_pending(pid_t t_id)
{
    PENDING * cursor = pending_head; 
    PENDING * previous = NULL; 
    // printf("CHecking for pending\n");
    while(cursor != NULL)
    {
        if((cursor->trader_id) == t_id)
        {
            // Found node
            if(previous == NULL)
            {
                // Head
                pending_head = cursor->next; 
            }
            else
            {
                previous->next = cursor->next; 
                // Not head 
            }
            return cursor; // Successful event removal 
        }
        previous = cursor; 
        cursor = cursor->next; 
    }
    // No event removal
    return NULL;
}

void free_pending(PENDING* head)
{
    PENDING * next = NULL;
    while(head != NULL)
    {
        next = head->next; 
        free(head);
        head = next;
    }
    head = NULL;
}

int main(int argc, char const *argv[])
{
    printf("%s Starting\n", LOG_PREFIX);
    /*
    ? Start by parsing arguments 
    argv[0] = product file 
    argv[1, n] = trader files
    */
    if (argc < 3)
    {
        perror("Incorrect number of args");
        return 1; 
    }

	/*
	? Make file pointer and read file
	*/
    FILE * fptr;
    int n_items;
    int64_t fees = 0;
    int i = 0;
    
    fptr = fopen(argv[1], "r");
    if(fptr == NULL)
    {
        perror("File not opened");
        return 1; 
    }

    if(fscanf(fptr, "%d", &n_items) != 1)
    {
        perror("Incorrect file structure");
        fclose(fptr);
        return 1;
    }

    fgetc(fptr); // Get rid of newline
    char ** contents = malloc(sizeof(char *) * n_items);

    // TODO: CHECK EACH PRODUCT IS UP TO 16 CHARS
 
	if(contents == NULL)
    {perror("Bruh "); fclose(fptr); return 1;}
    
    // ? Get items to trade
	while (!feof(fptr) && i < n_items)
    {
        char * line = malloc(LINE_SIZE);
        memset(line, 0, LINE_SIZE);
        fgets(line, LINE_SIZE, fptr);
        int pos = strcspn(line, "\n");
        if(pos < strlen(line))
        {
            line[pos] = '\0';
        }
        *(contents + i) = line;
        i++;
    }
    fclose(fptr);
    printf("%s Trading %d products:", LOG_PREFIX, n_items);
    for (i = 0; i < n_items; i++)
    {
        printf(" %s", *(contents + i));
    }
    printf("\n");

    // ? Initialising variables

    int trader_num = argc-2; // Number of traders 
    
    // ! LEAKS START HERE
    // ? Each trader (child) process's PID is stored here
    int * trader_pid = malloc(sizeof(int) * trader_num);

    // ? num of each trader's acquired objects stored here:
    TRADER ** traders = malloc(sizeof(TRADER*) * trader_num);
    for(i = 0; i < trader_num; i++)
    {
        TRADER * t_temp = malloc(sizeof(TRADER));
        t_temp->items = malloc(sizeof(int64_t) * n_items);
        memset(t_temp->items, 0, sizeof(int64_t)*n_items);
        t_temp->debt = malloc(sizeof(int64_t) * n_items);
        memset(t_temp->debt, 0, sizeof(int64_t)*n_items);
        *(traders + i) = t_temp;
    }

    // ? File path fo the named pipes 
    char ** exchange_pipes = malloc(sizeof(char*) * (trader_num));
    char ** trader_pipes = malloc(sizeof(char*) * (trader_num));
    
    // ? FDs of the pipes 
    int * trader_pipe_fds = malloc(sizeof(int) * trader_num);
    int * exchange_pipe_fds = malloc(sizeof(int) * trader_num);
    

    /*
	? Signal handler register. 
	? Should receive SIGUSR1 only. Exclude everything else
	*/
    struct sigaction exchange_handler; 
    memset(&exchange_handler, 0, sizeof(exchange_handler));
    exchange_handler.sa_flags = SA_SIGINFO;
    sigset_t s;
    siginfo_t info;
    sigemptyset(&s);
    sigaddset(&s, SIGUSR1);
    sigaddset(&s, SIGCHLD);

    
    
    /*
    ? EPOLL setup for event watching
    */
    int epoll_fd;
    struct epoll_event pipe_monitor;
    struct epoll_event events[MAX_EVENTS];
    char epoll_buffer[CMD_SIZE]; // What to read from
    
    /*
    ? Make epoll instance
    */
    epoll_fd = epoll_create1(0);
    
    /*
    ? Initializing stock engine
    ? Buy orders - sorted max->min with price [0]
    ? sell orders - sorted min->max with price [1]
    */
    ITEM ** engine = malloc(n_items * sizeof(ITEM*));
    for(i = 0; i < n_items; i++)
    {
        *(engine + i) = new_item(0,0);
    }

    /*
    ? each trader binary should be launched sequentially 
    ? Binaries are present in the argv
    ? Sequentially assign id's starting from 0
    ? fork, then exec()
    */
    for (i = 0; i < trader_num; i++)
    {
        /*
        ? Make 2 named pipes for each trader client
        TODO PUT THE PIPE STUFF INTO A FUNCTION
        */

        char *exchange_buff = malloc(LINE_SIZE);
        char *trader_buff = malloc(LINE_SIZE);
        
        memset(exchange_buff, 0, LINE_SIZE);
        memset(trader_buff, 0 , LINE_SIZE);


        snprintf(exchange_buff ,LINE_SIZE,FIFO_EXCHANGE, i); // ! CHANGE TO SNPRINTF
        snprintf(trader_buff, LINE_SIZE,FIFO_TRADER, i);
        
        mkfifo(exchange_buff, 0666);
        printf("%s Created FIFO %s\n", LOG_PREFIX, exchange_buff);

        mkfifo(trader_buff, 0666);
        printf("%s Created FIFO %s\n", LOG_PREFIX, trader_buff);

        *(exchange_pipes + i) = exchange_buff;
        *(trader_pipes + i) = trader_buff;

        /*
        ? Fork out the clients, and either connect if the parent is run,
        ? or exec to run the child
        */
        int temp_pid = fork();
        
        if(temp_pid < 0)
        {
            // ? Failure 
            perror("Fork fail");
            goto clear; 
            return 5;
        }

        // ? Executing child process
        else if(temp_pid == 0)
        {
            char temp[256];
            strcpy(temp, argv[i+2]);
            char to_add[16] = {0};
            sprintf(to_add, " %d", i);
            char *arguments[] = {temp, to_add, NULL};
            printf("%s Starting trader %d (%s)\n", LOG_PREFIX, i, argv[i+2]);
            if(execv(arguments[0], arguments) == -1)
            {
                perror("Child binary failed to launch");
                goto clear;
                return 1;
            }
            goto clear; 
            // ! Should never reach here
            return 1; 
        }
        
        /*
        ? Parent process
        */
        else
        {
            *(trader_pid + i) = temp_pid;
            // * (W) trader -> exchange (R) [trader_pipe]
            trader_pipe_fds[i] = open(*(trader_pipes + i), O_RDONLY);
            
            if(trader_pipe_fds[i] == -1) 
            {
                perror("BRUH MOMENT ERROR"); 
                printf("Open failed with errno: %d\n", errno);
                goto clear; 
                return 1;
            }
            
            /*
            ? Add each of the read pipe fd's to epoll to monitor
            */
            pipe_monitor.events = EPOLLIN | EPOLLHUP | EPOLLET; 
            pipe_monitor.data.fd = trader_pipe_fds[i];

            if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, trader_pipe_fds[i], 
                &pipe_monitor) == -1)
            {
                perror("trader_fd error: ");
                goto clear;
                return 1;
            }

            // * (W) exchange -> trader (R) [exchange pipe]
            exchange_pipe_fds[i] = open(*(exchange_pipes + i), O_WRONLY);
            if(exchange_pipe_fds[i] == -1) 
            {perror("exchange error"); goto clear; return 1;}
            
            printf("%s Connected to %s\n", LOG_PREFIX, *(exchange_pipes + i));
            printf("%s Connected to %s\n", LOG_PREFIX, *(trader_pipes + i));
            
            /*
            ? Sequentially send market open to traders
            ? Send kill signal afterwards
            */
            if(write(exchange_pipe_fds[i], MARKET_OPEN, 
                strlen(MARKET_OPEN)) < 0)
            {
                perror("Writing error");
                goto clear;
                return 1; 
            }
            kill(*(trader_pid + i), SIGUSR1);
        }
    
    }

    int num_events; 
    /*
    ? EVENT LOOP
    ? Every child is active at this point
    ? ALl set to 1. 
    ? When all children |, and == 0, we know every child has closed
    */
    int * active_traders = malloc(sizeof(int) * trader_num);
    memset(active_traders, 1, sizeof(int) * trader_num);

    /*
    ? These store the order id's of the traders.
    */
    int * trader_order_ids = malloc(sizeof(int) * trader_num);
    memset(trader_order_ids, 0, sizeof(int) * trader_num);
    sigprocmask(SIG_BLOCK, &s, NULL); // ! CHECK SIGNAL MASK
    unsigned long time_priority = 1;
    unsigned long saved_tp = 0;
    while(1)
    {

        if(!array_and(active_traders, trader_num))
        {
            break;
        }
        int signal = sigwaitinfo(&s, &info);
        struct epoll_event current_event;
        int found = 0;
        int trader_index;

        if(signal == SIGUSR1)
        {
            PENDING * test_event = remove_pending(info.si_pid); // Checks pending to see if any match
            
            if(test_event != NULL)
            {
                // There is an event pending that corresponds to the signal
                current_event = test_event->value;
                num_events = 1;
                found = 1;
                saved_tp = test_event->time_priority;
                free(test_event);
            }
            else
            {
                num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
                found = 0;
            }
        }
        else
        {
            num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            found = 0;
        }
        
        if(num_events == -1)
        {
            perror("epoll wait");
            goto clear;
            return 1;
        }
        // printf("\n\t\tPOLL RECEIVED: %d \n", num_events);

        // ? Checks for events only ever get 1 poll
        for (i = 0; i < num_events; i++)
        {
            // sigset_t pending_signals; 
            // sigpending(&pending_signals);
            if(!found)
            {
                current_event = events[i];
            }
            // ? Gets the index of the trader the event cooresponds to
            trader_index = index_of(trader_pipe_fds, 
                        trader_num, current_event.data.fd);
            if(trader_index < 0 || trader_index >= trader_num)
            {
                continue; 
            }
            if(info.si_pid != trader_pid[trader_index])
            {
                add_pending_event(events+i, trader_pid[trader_index], time_priority++);
                continue;
            }
            // ? Hangup - Trader has disonnected 
            if((current_event.events & EPOLLHUP))
            {
                int waiting_status; 
                int pid = waitpid(*(trader_pid + trader_index),
                    &waiting_status,0); // TODO WNOHANG??

                int dc_child = index_of(trader_pid, trader_num, pid);
                printf("%s Trader %d disconnected\n", 
                        LOG_PREFIX, dc_child);

                *(active_traders + trader_index) = 0;

                if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, 
                    *(trader_pipe_fds + dc_child), NULL)== -1)
                {
                    perror("Not deleted properly");
                    goto clear;
                }
                close(*(trader_pipe_fds + dc_child));
                close(*(exchange_pipe_fds + dc_child));
            }

            // ? Read in event
            if((current_event.events & EPOLLIN) && active_traders[trader_index])
            {
                // No signal sent
                memset(epoll_buffer, 0, sizeof(epoll_buffer));
                int read_bytes = read(current_event.data.fd, epoll_buffer, sizeof(epoll_buffer));
                if(read_bytes < 0)
                {
                    /*
                    ? Pipe reading messed up
                    */
                    perror("Error reading from pipe");
                    goto clear;
                }
                
                // ? Check if command ends with ';'
                if(check_msg(epoll_buffer, ';') != 1)
                {
                    // ? Failed the check: lacks a ';'
                    // ? Invalid, malformed or incomplete command
                    if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                    {
                        perror("Failure writing to pipe");
                        goto clear;
                    }
                    continue;
                }

                char to_print[CMD_SIZE];
                strcpy(to_print, epoll_buffer);
                
                for(int l = 0; l < strlen(to_print); l++)
                {
                    if(to_print[l] == ';')
                    {
                        to_print[l] = '\0';
                        break;
                    }
                }
                
                // * Buffer stores "<COMAND> <num>  <item>..."
                /*
                ? Deconstruct command and check validity 
                */
                printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, trader_index, to_print);
                char* cmd = strtok(epoll_buffer ," ");
                
                if(!valid_command(cmd))
                {
                    if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                    {
                        perror("Write failure");
                        goto clear;
                    }
                    continue;
                }
                char * order_id_str;
                int order_id;

                struct order* o = malloc(sizeof(struct order));
                o->command_name = cmd;
                int instruction_op = -1;
                /*
                buy = 0, sell = 1, amend = 2, cancel = 3, err = -1
                */
                char buy_or_sell_global; 
                time_priority++;

                if (!strcmp(cmd, "BUY"))
                {
                    // ? BUY operation
                    if(check_msg(to_print, ' ') != 4)
                    {
                        // ! Wrong buy msg
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    
                    // ? Check if order id is a valid order id
                    order_id_str = strtok(NULL, " ");
                    if(!valid_str(order_id_str))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        continue;
                    }

                    order_id = atoi(order_id_str);
                    if(!valid_int(order_id))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        continue;
                    }
                    
                    if((order_id < trader_order_ids[trader_index]) 
                    || (order_id != trader_order_ids[trader_index]))
                    {
                        // ! Non-sequential order id
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    o->order_id = order_id;
                    
                    // ? Get product name
                    o->product_name = strtok(NULL, " ");
                    // ? Check if product name is legit
                    if(!valid_product_name(o->product_name))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    // ? Get quantity 
                    char* tmp_qty = strtok(NULL, " ");
                    // ? Check if quantity is valid
                    if(tmp_qty == NULL || !check_if_integer(tmp_qty))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    o->qty = atoi(tmp_qty);
                    // ? Check if quantity is within the bounds
                    if(o->qty < 1 || o->qty > 999999)
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    // ? get price
                    char* tmp_price = strtok(NULL, ";");
                    // ? Check if price is valid
                    if(tmp_price == NULL || !check_if_integer(tmp_price))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    o->price = atoi(tmp_price);
                    // ? Check if price is in bounds
                    if(o->price < 1 || o->price > 999999)
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    // ? Get the item in question 
                    int index_of_item = find_item(contents, n_items, o->product_name);
                    // ? Check if item has been registered
                    if(index_of_item < 0)
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    if(found)
                    {
                        // Pending TP
                        add_buy(*(engine+index_of_item), o, trader_index, saved_tp);
                    }
                    else
                    {
                         // Just add TP normally
                        add_buy(*(engine+index_of_item), o, trader_index, time_priority);
                    }
                    trader_order_ids[trader_index]++;
                    instruction_op = 0;
                    buy_or_sell_global = 'b';
                }
                
                else if(!strcmp(cmd, "SELL"))
                {
                    if(check_msg(to_print, ' ') != 4)
                    {
                        // ! Wrong buy msg
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    order_id_str = strtok(NULL, " ");

                    if(!valid_str(order_id_str))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        continue;
                    }

                    order_id = atoi(order_id_str);
                    if(!valid_int(order_id))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        continue;
                    }

                    // ? Check if order id is a valid order id
                    if((order_id < trader_order_ids[trader_index]) 
                    || (order_id != trader_order_ids[trader_index]))
                    {
                        // ! Non-sequential order id
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    o->order_id = order_id;
                    
                    // ? Get product name
                    o->product_name = strtok(NULL, " ");
                    // ? Check if command name is legit
                    if(!valid_product_name(o->product_name))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    // ? Get quantity 
                    char* tmp_qty = strtok(NULL, " ");
                    // ? Check if quantity is valid
                    if(tmp_qty == NULL || !check_if_integer(tmp_qty))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    o->qty = atoi(tmp_qty);
                    // ? Check if quantity is within the bounds
                    if(o->qty < 1 || o->qty > 999999)
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    // ? get price
                    char* tmp_price = strtok(NULL, ";");
                    // ? Check if price is valid
                    if(tmp_price == NULL || !check_if_integer(tmp_price))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    o->price = atoi(tmp_price);
                    // ? Check if price is in bounds
                    if(o->price < 1 || o->price > 999999)
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    // ? Get the item in question 
                    int index_of_item = find_item(contents, n_items, o->product_name);
                    // ? Check if item has been registered
                    if(index_of_item < 0)
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    
                    if(found)
                    {
                        // Pending TP

                        add_sell(*(engine+index_of_item), o, trader_index, saved_tp);
                    }
                    else
                    {
                         // Just add TP normally
                        
                        add_sell(*(engine+index_of_item), o, trader_index, time_priority);
                    }

                    trader_order_ids[trader_index]++;
                    instruction_op = 1;
                    buy_or_sell_global = 's';
                }
                
                else if(!strcmp(cmd, "AMEND"))
                {
                    // AMEND operation 
                    // TODO Use trader_id and order_id
                    /*
                    ? To amend: Touch each ITEM 
                    ? Go through each PQ
                    ? Touch each node
                    ? Touch each descriptor and check if the order_id and trader_id match
                    ? Once the right match is found, remove that Descriptor, check the node
                    ? then add it back in as a buy or sell depending on what it was removed as
                    */
                    // ! PROBLEM?? 
                    // TODO CHECK ORDER ID IS ACTUALLY VALID/SENT BY TRADER
                    if(check_msg(to_print, ' ') != 3)
                    {
                        // ! Wrong buy msg
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }

                    order_id_str = strtok(NULL, " ");

                    if(!valid_str(order_id_str))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        continue;
                    }
                    
                    order_id = atoi(order_id_str);

                    if(!valid_int(order_id))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        continue;
                    }


                    char* tmp_qty = strtok(NULL, " ");
                    if(tmp_qty == NULL || !check_if_integer(tmp_qty))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            free(o);
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    int new_qty = atoi(tmp_qty);
                    o->qty = new_qty;

                    // ? Check quantity 
                    if(o->qty < 1 || o->qty > 999999)
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            free(o);
                            goto clear;
                        }
                        free(o);
                        continue;
                    }

                    char* tmp_price = strtok(NULL, ";");
                    if(tmp_price == NULL || !check_if_integer(tmp_price))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            free(o);
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    int new_price = atoi(tmp_price);
                    o->price = new_price;

                    if(new_price < 1 || new_price > 999999)
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            free(o);
                            goto clear;
                        }
                        free(o);
                        continue;
                    }
                    
                    DESC * descriptor_id = NULL;
                    NODE ** node_to_modify = malloc(sizeof(NODE*));
                    char buy_or_sell = '\0';
                    int item_index = -1;

                    for(item_index = 0; item_index < n_items; item_index++)
                    {
                        // TODO find descriptor
                        descriptor_id = find_descriptor(engine[item_index], trader_index, order_id, node_to_modify, &buy_or_sell);
                        if(descriptor_id != NULL)
                        {
                            break; 
                        }
                    }

                    if(descriptor_id == NULL)
                    {
                        // Descriptor not found, item does not exist.
                        if(write(exchange_pipe_fds[trader_index], 
                        INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            free(node_to_modify);
                            free(o);
                            goto clear;
                        }
                        free(node_to_modify);
                        free(o);
                        continue;
                    }
                    descriptor_id->trader_id = trader_index; 
                    // ? We have found the descriptor and parent node. 
                    ammend_operations(engine[item_index], *node_to_modify,
                    descriptor_id, new_price, new_qty, buy_or_sell, time_priority);
                    
                    free(node_to_modify);
                    instruction_op = 2;
                    o->product_name = contents[item_index];
                    buy_or_sell_global = buy_or_sell;
                }
                
                else if(!strcmp(cmd, "CANCEL"))
                {
                    // CANCEL operation 
                    /*
                    ? To cancel: Touch each ITEM 
                    ? Go through each PQ
                    ? Touch each node
                    ? Touch each descriptor and check if the order_id and trader_id match
                    ? Once the right match is found, remove that Descriptor, check the node
                    */
                    // TODO Use trader_id and order_id
                    // TODO REASSIGN COMMAND NAME TO BUY OR SELL DEPENDING ON THE ALLOCATION
                    if(check_msg(to_print, ' ') != 1)
                    {
                        // ! Wrong buy msg
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            goto clear;
                        }
                        free(o);
                        continue;
                    }

                    order_id_str = strtok(NULL, ";");

                    if(!valid_str(order_id_str))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        continue;
                    }

                    order_id = atoi(order_id_str);


                    if(!valid_int(order_id))
                    {
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Write failure");
                            goto clear;
                        }
                        continue;
                    }



                    DESC * descriptor_id = NULL;
                    NODE ** node_to_modify = malloc(sizeof(NODE*));
                    char buy_or_sell = '\0';
                    int item_index = -1;
                    for(item_index = 0; item_index < n_items; item_index++)
                    {
                        // TODO find descriptor
                        descriptor_id = find_descriptor(engine[item_index], trader_index, order_id, node_to_modify, &buy_or_sell);
                        if(descriptor_id != NULL)
                        {
                            break; 
                        }
                    }

                    if(descriptor_id == NULL)
                    {
                        // Descriptor not found, item does not exist yet. 
                        // printf("Order does not exist, cannot be cancelled\n");
                        if(write(exchange_pipe_fds[trader_index], INVALID_MSG, strlen(INVALID_MSG)) == -1)
                        {
                            perror("Failure writing to pipe");
                            free(o);
                            free(node_to_modify);
                            goto clear;
                        }
                        free(node_to_modify);
                        free(o);
                        continue;
                    }
                    // printf("Starting to cancel\n");
                    cancel_operations(engine[item_index], *node_to_modify, descriptor_id, buy_or_sell);
                    // printf("Cancel ended\n");
                    o->order_id = order_id;
                    instruction_op = 3;
                    free(node_to_modify);
                    o->product_name = contents[item_index];
                    buy_or_sell_global = buy_or_sell; 
                    o->qty = 0; 
                    o->price = 0;
                }
                
                else
                {
                    // TODO : MALFORMED COMMAND
                    instruction_op = 4; 
                    buy_or_sell_global = 'N';
                }
                /*
                ? Messages the trader that sent the command
                */
                char to_write[CMD_SIZE]; // This holds the "Accepted, ammended, cancelled, or invalid" messages
                memset(to_write, 0, sizeof(to_write));
                int to_write_len = 0;
                switch (instruction_op)
                {
                    case 0: // BUY
                    case 1: // SELL
                    {
                        to_write_len = snprintf(to_write, sizeof(to_write) 
                        ,ACCEPTED_MSG, order_id);
                        break;
                    }
                    

                    case 2: // AMEND
                    {
                        to_write_len = snprintf(to_write, sizeof(to_write) 
                        ,AMENDED_MSG, order_id);
                        break;
                    }

                    case 3: // Cancelled
                    {
                        to_write_len = snprintf(to_write, sizeof(to_write) 
                        ,CANCELLED_MSG, order_id);
                        break;
                    }

                    case -1:
                    {
                        to_write_len = snprintf(to_write, sizeof(to_write) 
                        ,INVALID_MSG);
                        break;
                    }
                    
                    default:
                        break;
                }
                
                if(to_write_len < 0)
                {
                    perror("Error formatting string\n");
                    free(o);
                    goto clear; 
                }

                /*
                ? Write to initial trader
                */
                if(write(*(exchange_pipe_fds + trader_index), to_write, strlen(to_write))==-1)
                {
                    perror("Bruh moment");
                    free(o);
                    goto clear; 
                }
                kill(*(trader_pid + trader_index), SIGUSR1);

                /*
                ? Write to all other traders 
                */
                for(int k = 0; k < trader_num; k++)
                {
                    if(k == trader_index)
                    {
                        continue;
                    }
                    memset(to_write, 0, sizeof(to_write));

                    if(buy_or_sell_global == 'b')
                    {
                        snprintf(to_write, sizeof(to_write),MARKET_MSG, "BUY", o->product_name, o->qty, o->price);
                    }
                    else if(buy_or_sell_global == 's')
                    {
                        snprintf(to_write, sizeof(to_write),MARKET_MSG, "SELL", o->product_name, o->qty, o->price);
                    }
                    else
                    {
                        perror("Wrong buy or sell");
                        free(o);
                        goto clear; 
                    }
                    
                    write(*(exchange_pipe_fds + k), to_write, strlen(to_write));
                    kill(*(trader_pid + k), SIGUSR1);
                }

                /*
                ? Match orders
                ? Take the minimum from each pq, and see if they match.
                * Pass in the right item to match
                */

                // Find most recent order using time priority
                if(instruction_op != 3)
                {
                    int item_index = find_item(contents, n_items, o->product_name);
                    /*
                    ? Iterate through each ITEM
                    */
                    fees += match_orders(*(engine + item_index), exchange_pipe_fds, 
                    trader_pid, trader_index, order_id, traders, item_index, active_traders);
                    // TODO: Process fees, update user order statistics
                }
                


                printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);
                for(int j = 0; j < n_items; j++)
                {
                    printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", 
                            LOG_PREFIX, *(contents + j), len_buy(*(engine + j)), len_sell(*(engine + j)));
                    
                    print_sell_nodes((*(engine + j))->sells);
                    print_buy_nodes((*(engine + j))->buys);
                }

                printf("%s\t--POSITIONS--\n", LOG_PREFIX);
                for(int j = 0; j < trader_num; j++)
                {
                    printf("%s\tTrader %d: ", LOG_PREFIX, j);
                    print_trader_items(traders[j], n_items, contents);
                    printf("\n");
                }
                
                free(o);
            }
        }
    }
    
    // ? Ending message
    printf("%s Trading completed\n", LOG_PREFIX);
    printf("%s Exchange fees collected: $%ld\n", LOG_PREFIX, fees);


    if(DBG)
    {
        for(i = 0; i < trader_num; i++)
        {
        DBG_PRINT("Child process with pid %d has stopped with status %d\n", pid, waiting_status);    
        }
    }
    

    clear: 
    // ? Clearing out memory
    free_pending(pending_head);
    for(i = 0; i < trader_num; i++)
    {
        free(traders[i]->items);
        free(traders[i]->debt);
        free(traders[i]);
    }
    free(traders);

    free(active_traders);

    free(trader_order_ids);
    for(int a = 0; a < n_items; a++)
    {
        free(*(contents + a));
    }

    for(i = 0; i < n_items; i++)
    {
        free_items(*(engine + i));
    }
    free(engine);


    free(contents);
    free(trader_pid);
    free(trader_pipe_fds);
    free(exchange_pipe_fds);
    

    for(i = 0; i < trader_num; i++)
    {
        unlink(*(exchange_pipes + i));
        unlink(*(trader_pipes + i));
    }

    for (i = 0; i < trader_num; i++)
    {
        free(*(exchange_pipes + i));
        free(*(trader_pipes + i));
    }
    
    free(trader_pipes);
    free(exchange_pipes);

    

    return 0;
}
