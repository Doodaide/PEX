#!/bin/bash 
echo ===================
echo RUNNING E2E TESTS: 
echo ===================

count=0
echo Test script: A sucessful test has no output. 
echo Unsuccessful will say failed
echo ___________________________________________

./test_folder/pe_exchange_test test_folder/products_test.txt ./test_folder/E2E_tests/trader_a_TEST > output.txt
# diff output.txt test_folder/E2E_tests/E2E_T1.out || echo "TEST 1 FAILED"
echo "Finished running 1 tests!"