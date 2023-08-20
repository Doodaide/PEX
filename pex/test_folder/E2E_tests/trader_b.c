#include "../../pe_trader.h"

volatile int sigusr_caught = 0; 

void sig_han(int signal_number)
{
    // printf("Signal caught: %d\n", sigusr_caught);
    sigusr_caught = 1;
    // printf("Signal after: %d\n", sigusr_caught);
}

int main(int argc, char const *argv[])
{

    if (argc < 1) {
        printf("Not enough arguments: argc = %d\n", argc);
        printf("Argv[0]: %s", argv[0]);
        return 1;
    } 
    

    // ! register signal handler
    int signal_received;
    struct sigaction sig_handle; 
    sig_handle.sa_flags = SA_SIGINFO;
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGUSR1);
    sig_handle.sa_handler = sig_han;
    sigaction(SIGUSR1, &sig_handle, NULL);
    
    
    // ! connect to named pipes
    int trader_id = atoi(argv[1]); // ! CHECK ERROR LATER

    int trader_fd = -1; // trader w -> r exchange
    int exchange_fd; // trader r <- w exchange
    
    char exchange_pipe[128] = {0};
    char trader_pipe[128] = {0};
    sprintf(exchange_pipe, FIFO_EXCHANGE, trader_id);
    sprintf(trader_pipe, FIFO_TRADER, trader_id);
    
    trader_fd = open(trader_pipe, O_WRONLY);
    if(trader_fd == -1)
    {
        perror("Trader open error");
        exit(1);
    }
    exchange_fd = open(exchange_pipe, O_RDONLY);
    if(exchange_fd == -1)
    {
        perror("Exchange open error");
        exit(1);
    }

    int counter = 0;
    int order_id = 0;
    int read_size;
    // int complete = 0;
    char buff[CMD_SIZE] = {0};
    char toSend[1064]; 
    // printf("Starting here: %d\n", sigusr_caught);
    if(sigusr_caught != 1)
    {
        pause();
    }
    sigusr_caught = 0;
    memset(buff, 0, sizeof(buff));
    // printf("a) Waiting for market open\n");
    read_size = read(exchange_fd, buff, sizeof(buff));
    if(counter == 0 && ((!strcmp("MARKET OPEN;", buff)))) // ! Market open signal
    {
        counter++;
        memset(buff, 0, sizeof(buff));
        // printf("a) Market open message received from exchange\n");
    }

    else
    {
        DBG_PRINT("Market open not first received: %s\n", buff);
        return 1;
    }
    sleep(5);
    

    /*
    char product_2[] = "SELL 1 PEAR 3 10;";
    if(write(trader_fd, product_2, strlen(product_2)) < 0)
    {
        perror("Writing product fail");
        return 1;
    }
    kill(getppid(), SIGUSR1);
    sleep(5);
    char product_3[] = "SELL 2 BANANA 1 9;";
    if(write(trader_fd, product_3, strlen(product_3)) < 0)
    {
        perror("Writing product fail");
        return 1;
    }
    kill(getppid(), SIGUSR1);
    sleep(5);

    char product_4[] = "AMEND 2 20 300;";
    if(write(trader_fd, product_4, strlen(product_4)) < 0)
    {
        perror("Writing product fail");
        return 1;
    }    
    kill(getppid(), SIGUSR1);
    
    sleep(5);

    

    char product_5[] = "CANCEL 2;";

    if(write(trader_fd, product_5, strlen(product_5)) < 0)
    {
        perror("Writing product fail");
        return 1;
    }
    kill(getppid(), SIGUSR1);
    sleep(5);
    */



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
