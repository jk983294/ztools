#include <iostream>
#include <string>
#include <zerg/tool/event_bus.h>

using namespace zerg;

struct MDSymbol {
  int a;
};

struct Order {
  int a3;
};

struct Trade {
  int a;
};

struct CustomEvent {
  std::string message;
  int value;
};

constexpr int32_t EVENT_TYPE_RAW_ORDER = 0;
constexpr int32_t EVENT_TYPE_RAW_TRADE = 1;
constexpr int32_t EVENT_TYPE_CUSTOM = 2;

using EB_CUSTOM_CB_t = std::function<void(int& to_change, const MDSymbol&, const CustomEvent*)>;
using EB_ORDER_CB_t = std::function<void(const MDSymbol&, const Order*)>;
using EB_TRADE_CB_t = std::function<void(const MDSymbol&, const Trade*)>;

struct MyApi {
  virtual ~MyApi() = default;
  virtual void OnOrder(const MDSymbol&, const Order*) = 0;
  virtual void OnTrade(const MDSymbol&, const Trade*) = 0;
};

struct Holder0 : public MyApi {
  ~Holder0() override = default;

  int getValue() const { return 42; }

  std::string getName() const { return "Holder0"; }

  void init(EventBus* bus) {
    m_bus = bus;
    m_bus->subscribe(EVENT_TYPE_RAW_ORDER, make_callback(this, &Holder0::OnOrder));
    m_bus->subscribe(EVENT_TYPE_RAW_TRADE, make_callback(this, &Holder0::OnTrade));
    m_bus->subscribe(EVENT_TYPE_CUSTOM, make_callback(this, &Holder0::OnCustomEvent));
  }

  void OnOrder(const MDSymbol&, const Order*) override { printf("OnOrder\n"); }

  void OnTrade(const MDSymbol&, const Trade*) override { printf("OnTrade\n"); }

  void OnCustomEvent(int& to_change, const MDSymbol&, const CustomEvent* event) {
    printf("OnCustomEvent: %s (value: %d)\n", event->message.c_str(), event->value);
    printf("Holder0 setting to_change from %d to %d\n", to_change, event->value);
    to_change = event->value;
  }

  EventBus* m_bus{nullptr};
};

struct Holder1 : public MyApi {
  ~Holder1() override = default;

  void init(EventBus* _bus) {
    m_bus = _bus;
    m_bus->subscribe(EVENT_TYPE_RAW_ORDER, make_callback(this, &Holder1::OnOrder));
    m_bus->subscribe(EVENT_TYPE_RAW_TRADE, make_callback(this, &Holder1::OnTrade));
  }

  void OnOrder(const MDSymbol&, const Order*) override { printf("OnOrder 1\n"); }

  void OnTrade(const MDSymbol&, const Trade*) override { printf("OnTrade 1\n"); }

  void OnCustomEvent(int& to_change, const MDSymbol&, const CustomEvent* event) {
    printf("OnCustomEvent1: %s (value: %d)\n", event->message.c_str(), event->value);
    printf("Holder1 setting to_change from %d to %d\n", to_change, event->value + 1);
    to_change = event->value + 1;
  }

  EventBus* m_bus{nullptr};
};

int main() {
  EventBus bus;
  Holder0 h0;
  h0.init(&bus);
  Holder1 h1;
  h1.init(&bus);

  printf("Testing make_callback with non-void return types:\n");
  auto int_callback = make_callback(&h0, &Holder0::getValue);
  auto string_callback = make_callback(&h0, &Holder0::getName);
  printf("getValue() returns: %d\n", int_callback());
  printf("getName() returns: %s\n", string_callback().c_str());
  printf("\n");

  MDSymbol symbol{1};
  Order order{3};
  Trade trade{4};
  CustomEvent customEvent{"Hello Custom Event", 42};

  printf("Firing ORDER event:\n");
  bus.fire_event<EB_ORDER_CB_t>(EVENT_TYPE_RAW_ORDER, symbol, &order);

  printf("Firing TRADE event:\n");
  bus.fire_event<EB_TRADE_CB_t>(EVENT_TYPE_RAW_TRADE, symbol, &trade);

  printf("Firing CUSTOM event:\n");
  int val = 41;
  printf("Value before CUSTOM event: %d\n", val);
  bus.fire_event<EB_CUSTOM_CB_t>(EVENT_TYPE_CUSTOM, val, symbol, &customEvent);
  printf("Value after CUSTOM event: %d\n", val);

  printf("Directly accessing callback vector size:\n");
  auto& orderCallbacks = bus.get_all_callbacks<EB_ORDER_CB_t>(EVENT_TYPE_RAW_ORDER);
  printf("Number of ORDER callbacks: %zu\n", orderCallbacks.size());
  for (auto& cb : orderCallbacks) {
    cb(symbol, &order);
  }

  auto& customCallbacks = bus.get_all_callbacks<EB_CUSTOM_CB_t>(EVENT_TYPE_CUSTOM);
  printf("Number of CUSTOM callbacks: %zu\n", customCallbacks.size());
  printf("EventBus demo completed successfully!\n");
  return 0;
}