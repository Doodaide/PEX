#include "pe_trader.h"

// ! NEED TO MAKE FAULT TOLERANT 

volatile int sigusr1_accepted = 0;
volatile int sigchld_accepted = 0;

void signal_handler(int signal_number)
{
    if(signal_number == SIGUSR1)
    {
        sigusr1_accepted = 1;
    }
    else if(signal_number == SIGCHLD)
    {
        sigchld_accepted = 1;
    }
}

int main(int argc, char ** argv) {
    
    if (argc < 2) {
        perror("Not enough arguments");
        return 1;
    }

    // ! register signal handler
    struct sigaction sig_handle;     
    memset(&sig_handle, 0, sizeof(sig_handle));
    sig_handle.sa_handler = signal_handler; 
    sig_handle.sa_flags = SA_SIGINFO;
    sigemptyset(&sig_handle.sa_mask);
    sigaction(SIGUSR1, &sig_handle, NULL);
    sigaction(SIGCHLD, &sig_handle, NULL);

    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGUSR1);
    sigaddset(&s, SIGCHLD); 

    /*
    struct timespec wait_time; 
    wait_time.tv_sec = 2; 
    wait_time.tv_nsec = 0; 
    */
    
    
    // ! connect to named pipes
    int trader_id = atoi(argv[1]); // ! CHECK ERROR LATER

    int trader_fd = -1; // trader w -> r exchange
    int exchange_fd; // trader r <- w exchange
    
    char exchange_pipe[CMD_SIZE] = {0};
    char trader_pipe[CMD_SIZE] = {0};

    sprintf(exchange_pipe, FIFO_EXCHANGE, trader_id);
    sprintf(trader_pipe, FIFO_TRADER, trader_id);

    exchange_fd = open(exchange_pipe, O_RDONLY);

    if(exchange_fd == -1)
    {
        perror("Exchange open error");
        exit(1);
    }

    trader_fd = open(trader_pipe, O_WRONLY);
    if(trader_fd == -1)
    {
        perror("Trader open error");
        exit(1);
    }

    int counter = 0;
    int order_id = 0;
    int read_size;
    // int complete = 0;
    char buff[CMD_SIZE] = {0};
    char to_send[CMD_SIZE]; 

    // ? Wait for market open signal: 
    // ? If market open signal is not received don't proceed
    
    while(1)
    {
        while(!sigusr1_accepted)
        {
            // pause();
            if(sigchld_accepted)
            {
                exit(1);
            }
        }
        sigusr1_accepted = 0;

        read_size = read(exchange_fd, buff, sizeof(buff));
        if(read_size < 0)
        {
            perror("read error: ");
            // return 1;
        }
        if(!strcmp("MARKET OPEN;", buff)) // ! Market open message
        {
            counter++;
            memset(buff, 0, sizeof(buff));
            break;
        }
    }
    
    /*
    ? Event loop: Checks the messages
    */

    while(1)
    {
        memset(buff, 0, sizeof(buff));
        int write_success = 0;
        // Message is done
        while(!sigusr1_accepted)
        {
            // pause();
            if(sigchld_accepted)
            {
                exit(1);
            }
        }
        sigusr1_accepted = 0;

        read_size = read(exchange_fd, buff, sizeof(buff));
        if(read_size < 0)
        {
            perror("read error: ");
            // return 1;
        }
        /*
        ? Trader can receive: 
        ? MARKET BUY...;, MARKET SELL...;
        ? INVALID; 
        ? ACCEPTED <order_id>;
        ? CANCELLED <ORDER_id>;
        ? AMENDED <ORDER_ID>;
        ? FILL <ORDER_ID> <QTY>;
        */

        if(check_msg(buff, ';') != 1)
        {
            // Invalid message sent
            continue; 
        }

        if(!strcmp(buff, "INVALID;"))
        {

            // Invalid order received, pass it on; 
            // TODO: Consider if we receive this do we resend? 
            continue; 
        }
        
        char * tok_1 = strtok(buff, " ");
        if(tok_1 == NULL || is_empty(tok_1))
        {
            continue;
        }

        if(!strcmp(tok_1, "FILL"))
        {
            continue; 
        }

        if(!strcmp(tok_1, "MARKET"))
        {
            // Market buy or market sell. THis one we should be worried about
            char * tok_2 = strtok(NULL, " ");
            if(!strcmp(tok_2, "SELL"))
            {
                memset(to_send, 0, sizeof(to_send));
                char * product_name = strtok(NULL, " ");
                if(!valid_product_name(product_name))
                {
                    continue;
                }
                char * temp_qty = strtok(NULL, " ");
                if(temp_qty == NULL || !check_if_integer(temp_qty))
                {
                    continue; 
                }
                int qty_numerical = atoi(temp_qty);
                if(qty_numerical >= 1000 || qty_numerical < 1)
                {
                    perror("Unexpected");
                    close(trader_fd);
                    close(exchange_fd);
                    return 1;
                }
                char * temp_price = strtok(NULL, ";");
                if(temp_price == NULL || !check_if_integer(temp_price))
                {
                    continue;
                }
                int price_numerical = atoi(temp_price);
                if(price_numerical < 1 || price_numerical > 999999)
                {
                    continue; 
                }
                sprintf(to_send, "BUY %d %s %d %d;", order_id, 
                product_name, qty_numerical, price_numerical);

                write_success = write(trader_fd, to_send, strlen(to_send));
            
                if(write_success < 0)
                {
                    perror("Writing fail");
                    close(trader_fd);
                    close(exchange_fd);
                    return 1;
                }
                kill(getppid(), SIGUSR1);
                
                // ? Wait for accepted: 

                int accepted_received = 0;
                while(!accepted_received)
                {
                    char accepted_buff[CMD_SIZE];
                    memset(accepted_buff, 0, sizeof(accepted_buff));
                    // Message is done
                    while(!sigusr1_accepted)
                    {
                        sleep(2);
                        if(sigchld_accepted)
                        {
                            exit(0);
                        }
                        if(!sigusr1_accepted)
                        {
                            // ? Accepted not received in time. Resend order. 
                            kill(getppid(), SIGUSR1);
                            continue; 
                        }
                        // signal = sigtimedwait(&s, &signal_received, &wait_time);
                    }
                    
                    sigusr1_accepted = 0;
                    // ? Signal was received by trader
                    read_size = read(exchange_fd, accepted_buff, sizeof(accepted_buff));
                    if(read_size < 0)
                    {
                        perror("read error: ");
                        // return 1;
                    }
                    char * accepted = strtok(accepted_buff, " ");
                    if(accepted == NULL)
                    {
                        continue;
                    }
                    char * temp_order_id = strtok(NULL, ";");
                    if(temp_order_id == NULL || !check_if_integer(temp_order_id))
                    {
                        continue; 
                    }
                    int accepted_order_id = atoi(temp_order_id);
                    
                    if(order_id != accepted_order_id)
                    {
                        continue;
                    }
                    if(!strcmp(accepted, "ACCEPTED"))
                    {
                        order_id ++; 
                        accepted_received = 1; 
                        break;
                    }
                }                
            }
        }
    }
    close(trader_fd);
    close(exchange_fd);
    unlink(exchange_pipe);
    unlink(trader_pipe);
    // event loop:

    // wait for exchange update (MARKET message)
    // send order
    // wait for exchange confirmation (ACCEPTED message)
    return 0;
}