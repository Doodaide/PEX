TARGET=pe_exchange
TRADER=pe_trader

CC=gcc
# CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g 
#ASAN_FLAGS=-fsanitize=address,leak
ASAN_FLAGS= 
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g
# valgrind --leak-check=full --show-leak-kinds=all --trace-children=no -s ./pe_exchange products.txt ./trader_a


LDFLAGS=-lm

SRC =get_command.c matching_engine.c pe_exchange.c pe_trader.c 

OBJ = $(SRC:.c=.o)

all:$(TARGET) $(TRADER)

$(TARGET):$(OBJ)
	$(CC) $(ASAN_FLAGS) -o $@ get_command.o matching_engine.o pe_exchange.o 

$(TRADER):$(OBJ)
	$(CC) $(ASAN_FLAGS) -o $@ get_command.o pe_trader.o

.SUFFIXES: .c .o 

.c .o: 
	$(CC) $(CFLAGS) $< 

.PHONY: clean

clear:
	rm /tmp/pe_exchange_1 /tmp/pe_trader_1 /tmp/pe_exchange_0 /tmp/pe_trader_0 

commit:
	git commit -a -m "Bruh"
	git push

clean:
	rm -f $(TARGET) $(TRADER) *.o trader_a trader_b

tests: 
	bash test_folder/test_maker.sh 

run_tests: 
	bash test_folder/test_runner.sh
	bash test_folder/E2Etest_runner.sh