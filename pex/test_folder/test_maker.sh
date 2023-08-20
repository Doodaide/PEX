#!/bin/bash 

# Bash script compiles everything with the right flags 
echo =====================
echo MAKING TESTS: 
echo =====================
echo MAKING BINARY OBJECTS 
gcc -c -o test_folder/get_command.o get_command.c # Command pallete 
gcc -c -o test_folder/matching_engine.o matching_engine.c # Matching engine 
gcc -c -o test_folder/pe_exchange.o pe_exchange.c # exchange main code 
gcc -c -o test_folder/pe_trader.o pe_trader.c # autotrader main code 
gcc -c -o test_folder/test_binary.o test_folder/unit-tests.c

echo MAKING BINARIES 
gcc -o test_folder/pe_exchange_test test_folder/get_command.o test_folder/matching_engine.o test_folder/pe_exchange.o # exchange binary 
gcc -o test_folder/pe_trader_test test_folder/get_command.o test_folder/pe_trader.o # autotrader binary 

echo MAKING UNIT TEST BINARY 
gcc -o test_folder/test_binary_test test_folder/test_binary.o test_folder/get_command.o test_folder/matching_engine.o -lm test_folder/libcmocka-static.a

echo MAKING E2E TEST BINARY 
gcc test_folder/E2E_tests/trader_a.c get_command.c -o test_folder/E2E_tests/trader_a_TEST
gcc test_folder/E2E_tests/trader_b.c get_command.c -o test_folder/E2E_tests/trader_b_TEST