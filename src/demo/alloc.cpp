#include <iostream>
#include <zerg/tool/mem_pool.h>

using namespace zerg;

struct Order {
  int id;
  double price;
  int quantity;
};

struct TradeOpportunity {
  Order* buy_order;
  Order* sell_order;
  double profit;
};

void process_market_update(const char* raw_data, size_t data_size) {
  constexpr size_t ALLOC_SIZE = 10 * 1024;  // 10KB per processing window
  StackAllocator local_alloc(ALLOC_SIZE);

  // Mark start of processing
  void* mark = local_alloc.mark();

  // Parse raw data into order book (temporary objects)
  int num_orders = 5;
  Order* orders = static_cast<Order*>(local_alloc.allocate(num_orders * sizeof(Order)));

  for (int i = 0; i < num_orders; ++i) {
    orders[i] = {1000 + i, 150.25 + i * 0.1, 100 - i * 10};
  }

  // Find arbitrage opportunities
  TradeOpportunity* opps =
    static_cast<TradeOpportunity*>(local_alloc.allocate((num_orders / 2) * sizeof(TradeOpportunity)));

  int opp_count = 0;
  for (int i = 0; i < num_orders; i += 2) {
    opps[opp_count++] = {&orders[i], &orders[i + 1], orders[i].price - orders[i + 1].price};
  }

  // Execute trades (simulated)
  for (int i = 0; i < opp_count; ++i) {
    std::cout << "Executing trade: Profit=" << opps[i].profit << "\n";
  }

  // Risk calculation (temporary buffer)
  double* risk_buffer = static_cast<double*>(local_alloc.allocate(3 * sizeof(double)));
  risk_buffer[0] = 0.02;   // VaR
  risk_buffer[1] = 0.95;   // Confidence
  risk_buffer[2] = 1.5e6;  // Exposure

  // ... more processing ...

  // Single operation to free ALL temporaries
  local_alloc.deallocate(mark);
}

int main() {
  char market_data[] = "MSFT,BID,150.25;ASK,150.30;...";

  // Process 1000 updates
  for (int i = 0; i < 1000; ++i) {
    process_market_update(market_data, sizeof(market_data));
  }

  std::cout << "Processed 1000 market updates\n";
  return 0;
}