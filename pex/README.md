1. Describe how your exchange works.
My exchange holds pointers to each item in a dynamically allocated block of memory. Each item has two priority queues (PQs)(implemented as singly linked lists) that represents the prices. Buy orders have a max PQ, and sells a min PQ. 

Each priority queue is ordered by order price, and has a smaller linked PQ branching off each price that represents distinct orders. These orders are arranged in terms of time priority as they are all the same price. Every time a new order is sent (whether that be buy, sell, amend), the matching engine algorithm will check the item that was most recenlty updated, and assess the two highest nodes in the PQs. 

If they are matchable based upon the criteria listed in the spec, then they will be popped out and updated. This is acheived through a combination of linked list updating operations. Finally, each trader is written to and their attributes are updated (each trader is represented by a struct that holds key information such as the quantity of products it holds, debt, etc.). 

2. Describe your design decisions for the trader and how it's fault-tolerant.

The autotrader (if simplified) essentially only cares about 1 major command, that being MARKET SELL..., which should be parsed, and be echoed back to the exchange. Now, my autotrader will then enter a while loop that waits for an "ACCEPTED" message from the exchange. If an "ACCEPTED" is received (with the signal), then the trader can move on. However, if a signal of confirmation is not received within 2 seconds, the trader sends another signal through to the exchange. 

3. Describe your tests and how to run them.
Use `make tests` to make all of the tests, and `make run_tests` to run the tests. 
There are 2 sets of tests that are run: unit tests and End to End tests. 
The unit tests have been designed using the cMocka framework. The unit tests test individual functions' functionalities. 
The End to End tests examine several trading scenarios with some dummy traders. They send various signals, trades, and requests to the exchange testing its capabilities. However, I did not use diff as it refused to work for some reason