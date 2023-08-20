while(1)
    {
        sigwait(&s, &signal_received);
        memset(buff, 0, sizeof(buff));
        printf("Message received!!\n");
        read_size = read(exchange_fd, buff, sizeof(buff));
        printf("read successful in child\n");
        if(counter == 0 && ((!strcmp("MARKET OPEN;", buff)))) // ! Market open signal
        {
            counter++;
            memset(buff, 0, sizeof(buff));
            printf("Market open message received from exchange\n");
            continue;
        }
        printf("Passing market open check\n");


        if(read_size < 0)
        {
            perror("read error: ");
            // return 1;
        }

        int write_success = 0;
        // Message is done
        struct order * msg_received = get_command(buff); 
        if(!strcmp(msg_received->command_name, "AMMENDED")
        || !strcmp(msg_received->command_name, "CANCELLED")
        || !strcmp(msg_received->command_name, "INVALID"))
        {
            free(msg_received);
            continue;
        }
        if(!strcmp(msg_received->command_name, "ACCEPTED"))
        {
            free(msg_received);
            order_id++;
            continue;
        }

        else if(!strcmp("SELL", msg_received->command_name))
        {
            memset(toSend, 0, sizeof(toSend));
            if(msg_received->qty >= 1000)
            {
                free(msg_received);
                perror("Unexpected");
                close(trader_fd);
                close(exchange_fd);
                return 1;
            }
            sprintf(toSend, "BUY %d %s %d %d;", order_id, 
            msg_received->product_name, msg_received->qty, msg_received->price);

            write_success = write(trader_fd, toSend, strlen(toSend));
            
            if(write_success < 0)
            {
                //kill(getppid(), SIGKILL);
                perror("Writing fail");
                close(trader_fd);
                close(exchange_fd);
                return 1;
            }
            kill(getppid(), SIGUSR1);
            free(msg_received);
        }
        //printf("Reaching here\n");
        //command_end = 0;
        counter ++;
        
    }