#include "order_book.hpp"


int OrderBook::add_order(const Order& o) {
        // Step 1: determine if bid or ask
	PriceLevelArray& target = (o.side == Side::BID) ? bids : asks;
	/* Number of orders per price level */
	size_t order_block = target.max_orders_per_level;
        size_t i = 0;
	/* Target is not a reference, therefore there is no need to deferenciate */
        /* Scorri per il target array e prova ad inserire l'ordine */
        while (i < target.max_levels && target.order_count[i] > 0) {
                /* Make appropriate price comparison and decide if insert */
                if (target.prices[i] == o.price) {
                        /* User order refers to an alreasy existing price level: insert here */
                        /* Position where to copy the order */
                        target.orders[order_block * i + target.order_count[i]] = o;
                        target.order_count[i]++;
                        return 1;
                /* Otherwise, scroll until inequality is violated, insert then here */
                /* The idea is that the prices monotonic behavior must be preserved */
                } else if (o.side == Side::ASK ?
                                (target.prices[i] > o.price):
                                (target.prices[i] < o.price)) {
                        /* Shift all the existing pricelevels */
                        for (size_t j = target.max_levels - 1; j > i; --j) {
                                target.prices[j] = target.prices[j-1];
                                target.order_count[j] = target.order_count[j-1];
                                /* copy order_block (j-1) in j */
                                for (int k = target.order_count[j] - 1;
                                        k >= 0; --k) {
                                        target.orders[j * order_block + k] =
                                                target.orders[(j-1) * order_block + k];
                                }
                        }
                        /*  add the order here, as first for this price */
                        target.prices[i] = o.price;
                        target.orders[order_block * i] = o;
                        target.order_count[i] = 1;
                        if (o.side == Side::BID) {
                                bid_count++;
                        } else {
                                ask_count++;
                        }
                        return 1;
                } else {
                        /* Just proceed */
                }
                /* When insert, then exit the function with 1 */
                i++;
        }
        /* Post-while */
        if (i < target.max_levels) {
                /* It means that the PriceLevel i was empty: add first order */
                target.orders[order_block * i] = o;
                target.prices[i] = o.price;
                target.order_count[i] = 1;
                if (o.side == Side::BID) {
                        bid_count++;
                } else {
                        ask_count++;
                }
                return 1;
        }
        /* If for some reason inserting was not possible, return 0 */
        return 0;
}



std::optional<OrderLocation> OrderBook::find_order(uint64_t order_id) {
	/* Look for the order having the requested id. Returns a struct containing
	   the position if found, otherwise {}, correctly interpreted under che C++17 standard */
        PriceLevelArray* tt = &asks;
	OrderLocation found_order;
	found_order.side = Side::ASK;
        for (int t = 0; t < 2; ++t) {
		if (t == 1) {tt = &bids; found_order.side = Side::BID;}
                /* Scroll all the positive order_count */
                for(size_t i = 0; i < tt->max_levels; ++i) {
                        if(tt->order_count[i] > 0) {
                                /* Look for the order, and if found, returns */
                                for(size_t k=0; k < tt->order_count[i]; ++k) {
                                        if(tt->orders[(tt->max_orders_per_level)*i+k].id == order_id) {
						found_order.prices_offset = i;
						found_order.orders_offset = k;
						return found_order;
                                        }
				}
			}
		}
        }
	/* Order not found */
	return {};
}


int OrderBook::cancel_order(uint64_t order_id) {
	auto is_found = find_order(order_id);
	if (is_found) {
		/* Ok is found, store its coordinates */
		OrderLocation my_coords = is_found.value();
		PriceLevelArray& tt = (my_coords.side == Side::ASK ? asks : bids);
		size_t i = my_coords.prices_offset;
		size_t k = my_coords.orders_offset;
		/* Now go there and remove the order: override it with all next */
                for(size_t n=k; n < tt.max_orders_per_level - 1; ++n) {
                	tt.orders[(tt.max_orders_per_level)*i+n] = 
				tt.orders[(tt.max_orders_per_level)*i+n+1];
                }
		tt.order_count[i]--;
                /* Clean memory part just after shift */
                memset(&(tt.orders[tt.max_orders_per_level*i+tt.order_count[i]]), 0, sizeof(Order));
		/* if zero, the level has been made empry:shift all the remainig levels back */
               	if (tt.order_count[i] == 0) {
			/* Shift back all the remaining: the very last one is not yet touched*/
			for (size_t j = i; j < tt.max_levels - 1; ++j) {
				tt.prices[j] = tt.prices[j+1];
                                tt.order_count[j]=tt.order_count[j+1];
				/* Shift the order blocks */
				for(size_t kk=0; kk < tt.max_orders_per_level; ++kk) {
                                	tt.orders[tt.max_orders_per_level*j+kk] = 
						tt.orders[tt.max_orders_per_level*(j+1) + kk];
                                }
                        }
                        /* Now, reset the very last position (no shift, just set to zero) */
                        tt.prices[tt.max_levels - 1] = 0;
                        tt.order_count[tt.max_levels - 1] = 0;
			/* Clean the very last order block */
                        memset(&(tt.orders[(tt.max_levels - 1)*(tt.max_orders_per_level)]), 
				0,
				sizeof(Order) * (tt.max_orders_per_level));
                        /* Decrease the price level tracker */
                        if (my_coords.side == Side::ASK) ask_count--;
                        else bid_count--;
                }
		return 1;
	} else {
		return 0;
	}
}


int OrderBook::modify_order(uint64_t order_id, uint32_t new_volume) {
	/* First: localize the order */
	auto is_found = find_order(order_id);
	if (is_found) {
		OrderLocation my_coords = is_found.value();
		PriceLevelArray& tt = (my_coords.side == Side::ASK ? asks : bids);
		size_t i = my_coords.prices_offset;
		size_t k = my_coords.orders_offset;
		/* Modify the order */
		if(tt.orders[tt.max_orders_per_level*i+k].volume >= new_volume) {
			/* Simply update, no problem */
                        tt.orders[tt.max_orders_per_level*i+k].volume = new_volume;
                        return 1;
                } else {
                       /* Timestamp must be updated */
                       Order tmp = tt.orders[tt.max_orders_per_level * i + k];
                       tmp.volume = new_volume;
                       tmp.timestamp = get_timestamp();
                       cancel_order(order_id);
                       add_order(tmp);
                       return 1;
              	}
	} else {
		return 0;
	}
}

uint64_t get_timestamp() {
	/* Returns the time elapsed since epoch, in nanoseconds.
	   IMPORTANT! epoch is usually the system boot. It is not guaranteed and
	   implementation-dependent, but what here matters is the monotonic behavior */
	/* Get the class able to count time in a guaranteed monotonic way */
	auto now = std::chrono::steady_clock::now();
	/* Return the number of nanoseconds elapsed since epoch */
	auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>
		(now.time_since_epoch()).count();
	return static_cast<uint64_t>(ns);
}


int OrderBook::match_orders(std::vector<Trade>& trades) {
	int still_matches = 1;
        int performed = 0;
        while (still_matches) {
                /* Navigate the order book and perform all possible matches */
                if (asks.prices[0] > 0 && bids.prices[0] > 0 &&
                        asks.prices[0] <= bids.prices[0]) {
                        /* There is a match: perform it */
			/* Add a new vector element */
			trades.emplace_back();
			/* Refers to such an element, avoiding the creation of local copies */
	                Trade& ct = trades.back();
                        /* Retreive info on the ask and bid order */
                        ct.ask_id = asks.orders[0].id;
                        ct.bid_id = bids.orders[0].id;
                        ct.price = asks.prices[0];
                        ct.timestamp = get_timestamp();
                        /* Now determine how to handle volumes */
                        if (asks.orders[0].volume == bids.orders[0].volume) {
                                /* Simplest case: requires no further adjustements */
                                ct.volume = asks.orders[0].volume;
                                /* Simply remove both orders from the book */
                                cancel_order(ct.ask_id);
                                cancel_order(ct.bid_id);
                        } else if (asks.orders[0].volume > bids.orders[0].volume) {
                                /* Execute full bids, but there are still remaining asks */
                                uint32_t ask_vol = asks.orders[0].volume - bids.orders[0].volume;
                                ct.volume = bids.orders[0].volume;
                                cancel_order(ct.bid_id);
                                modify_order(ct.ask_id, ask_vol);
                        } else {
                                /* Execute full asks, but there are still remaining bids */
                                uint32_t bid_vol = bids.orders[0].volume - asks.orders[0].volume;
                                ct.volume = asks.orders[0].volume;
                                cancel_order(ct.ask_id);
                                modify_order(ct.bid_id, bid_vol);
                        }
                        performed++;
                } else {
                still_matches = 0;
                }
        }
        return performed;
}


/* Simple tools to display variable values */
uint64_t OrderBook::best_ask_price() const {return asks.prices[0];}
uint64_t OrderBook::best_bid_price() const {return bids.prices[0];}
uint32_t OrderBook::get_ask_count() const {return ask_count;}
uint32_t OrderBook::get_bid_count() const {return bid_count;}

/* << operators, left as free functions to preserve compatibility with cout << */
std::ostream& operator<<(std::ostream& os, const Order& o) {
	os << "Order:{id=" << o.id
	   << ", price=" << o.price 
	   << ", volume=" << o.volume
           << ", side=" << (o.side == Side::ASK ? "ASK" : "BID")
           << "}";
    return os;
}


std::ostream& operator<<(std::ostream& os, const OrderBook& ob) {
	os << "OrderBook STATUS: {"
	   << "ASK: [Best "
	   << ob.best_ask_price()
	   << " , Active "
	   << ob.get_ask_count()
	   << "] *** BID: [Best "
	   << ob.best_bid_price()
	   << " , Active "
	   << ob.get_bid_count()
	   << "]}";
	return os;
}


std::ostream& operator<<(std::ostream& os, const Trade& trade) {
	os << "Trade {ask_id: " 
	   << trade.ask_id 
	   << ", bid_id: "
	   << trade.bid_id
	   << " price "
	   << trade.price
	   << ". time "
	   << trade.timestamp
	   << " vol "
	   << trade.volume
	   << "}";
	return os;
}

