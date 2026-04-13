#pragma once
#include<iostream>
// header stdint.h taken from C.
#include<cstdint>
#include<cassert>
#include<vector>
#include<optional>
#include<cstring>
/* Used to get the current timestamp in nanoseconds */
#include<chrono>

/* Still to define: PriceLevelArray, Order, Trade */
/* Remember the difference w.r.t. the enum in C */
enum class Side: uint8_t {ASK = 0, BID = 1};

/* In C++11 default values for structure members are allowed */
/* Furthermore, it is no more necessary to use typedef. */
struct Order {
	uint64_t price		= 0;
	uint64_t id		= 0;
	uint64_t timestamp 	= 0; 
	uint32_t volume		= 0;
	Side side		= Side::ASK;
};

/* Store a successful match */
struct Trade {
	uint64_t ask_id 	= 0;
	uint64_t bid_id		= 0;
	uint64_t price		= 0;
	uint64_t timestamp	= 0;
	uint32_t volume		= 0;
};


/* Store information about where an order is located */
struct OrderLocation {
	size_t prices_offset	= 0;
	size_t orders_offset	= 0;
	Side side		= Side::ASK;
};


class PriceLevelArray {
	friend class OrderBook;
private:
	size_t max_levels;
	size_t max_orders_per_level;
	std::vector <uint64_t> prices;
	std::vector <Order> orders;
	std::vector <uint32_t> order_count;
public:
	/* Constructor with member pre-inizialization */
	PriceLevelArray(int max_levels = 100, int max_orders_per_level = 100):
		max_levels(static_cast<size_t>(max_levels)),
		max_orders_per_level(static_cast<size_t>(max_orders_per_level)),
		prices(max_levels),
		orders(max_levels * max_orders_per_level),
		order_count(max_levels) 
		/* To avoid edge cases, ensure to have at least 2 levels and 2 blocks per level */
		{ assert(max_levels >= 2 && max_orders_per_level >= 2); };
	/* Do not allow *copy*, but allow *move* */
	PriceLevelArray(const PriceLevelArray&) = delete;
	PriceLevelArray& operator=(const PriceLevelArray&) = delete;
	PriceLevelArray(PriceLevelArray&&) = default;
	PriceLevelArray& operator=(PriceLevelArray&&) = default;
};


class OrderBook {
private:
	PriceLevelArray asks;
	PriceLevelArray bids;
	uint32_t ask_count;
	uint32_t bid_count;
	std::optional<OrderLocation> find_order(uint64_t order_id);
public:
	/* Using default arguments: allowed from C++11 onwards */
	OrderBook(int max_levels = 100, int max_orders_per_level = 100):
		asks(max_levels, max_orders_per_level),
		bids(max_levels, max_orders_per_level),
		ask_count(0),
		bid_count(0)
		{};
	/* Constructor activated when using the '=' operator. Here, we are denying copy. */
	/* First deny: when using OrderBook ob1(ob2); */
	OrderBook(const OrderBook&) = delete;
	/* Second deny: when using OrderBook ob1 = ob2; */
	OrderBook& operator=(const OrderBook&) = delete;
		
	/* Since we denied a constructor, we must explicitely say which are allowed */
	/* Constructor activated when using the std::move() function, allowed  */
	OrderBook(OrderBook&&) = default;
	OrderBook& operator=(OrderBook&&) = default;

	/* Methods */	
	int add_order (const Order& o);
	int match_orders(std::vector<Trade>& trades);
	int modify_order(uint64_t order_id, uint32_t new_volume);	
	int cancel_order(uint64_t order_id);

	/* Simple tools to display variable values */
	uint64_t best_ask_price() const;
	uint64_t best_bid_price() const;
	uint32_t get_ask_count() const;
	uint32_t get_bid_count() const;
	

};


/* streming output overloading */
/* Allow the use of cout << Order << */
std::ostream& operator<<(std::ostream& os, const Order& o);
/* Allow the use of cout << OrderBook << ... */
std::ostream& operator<<(std::ostream& os, const OrderBook& ob);
/* Allow the use of cout << Trade << ... */
std::ostream& operator<<(std::ostream& os, const Trade& trade);

uint64_t get_timestamp();
