CC=g++
CFLAGS=-std=c++17 -O2 -Wall -Wextra -g

orderbook: main.cpp order_book.cpp order_book.hpp
	$(CC) $(CFLAGS) main.cpp -o orderbook order_book.cpp


.PHONY: clean

clean:
	rm orderbook
