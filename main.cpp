#include <iostream>
#include <random>
#include "order_book.hpp"

constexpr int MAX_ORDERS = 1000;

/* C++ does not require (void) */
int main() {

	/* Generate an rng with a fixed seed */
	std::mt19937 rng(0);

	/* Define the probability distributions */
	std::uniform_int_distribution<uint64_t> price_dist(1, 20);
	std::uniform_int_distribution<uint32_t> volume_dist(1, 10);
	std::uniform_int_distribution<int> side_dist(0, 1);

	std::vector<Trade> my_trades;
	my_trades.reserve(20);
	uint64_t end = 0;
	int trades_done = 0;

	OrderBook ob;
//	std::cout << "STARTING POSITION" << std::endl;
//	std::cout << ob << std::endl;
	std::cout << "GENERATING " << MAX_ORDERS << " RANDOM ORDERS\n";

	uint64_t start = get_timestamp();
	for (int i = 0; i < MAX_ORDERS; ++i) {
		/* Generate a new random order and print it */
		Order tmp;
		tmp.price = price_dist(rng);
		tmp.id = i;
		tmp.timestamp = get_timestamp();
		tmp.volume = volume_dist(rng);
		tmp.side = side_dist(rng) ? Side::BID : Side::ASK;
		/* ostream to be removed when measung performance */
//		std::cout << tmp << std::endl;
		ob.add_order(tmp);
	}
//	std::cout << ob << std::endl;

	/* Perform the matches and print a summary */
	trades_done = ob.match_orders(my_trades);
	end = get_timestamp();

	std::cout << MAX_ORDERS << " orders processed and "
		  << trades_done
		  << " trades done in "
		  << end - start << " nanoseconds ("
		  << (static_cast<double>(end - start)) / 1000000000
		  << " seconds)"
		  << std::endl;

	/* Print a summary of the current OrderBook status
	 * and the performed trades */
//	std::cout << ob << std::endl;
//	for (const auto& t : my_trades) {
//   		std::cout << t << "\n";
//	}

	return 0;
}
