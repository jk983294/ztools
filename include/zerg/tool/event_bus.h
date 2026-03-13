#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>

namespace zerg {
template <typename T>
struct member_function_traits;

template <typename ReturnType, typename ClassType, typename... Args>
struct member_function_traits<ReturnType (ClassType::*)(Args...)> {
  using signature = ReturnType(Args...);
  using callback_type = std::function<ReturnType(Args...)>;
};

template <typename ReturnType, typename ClassType, typename... Args>
struct member_function_traits<ReturnType (ClassType::*)(Args...) const> {
  using signature = ReturnType(Args...);
  using callback_type = std::function<ReturnType(Args...)>;
};

template <typename T, typename MemberFunc>
auto make_callback(T* instance, MemberFunc member_func) {
  using traits = member_function_traits<MemberFunc>;
  using CallbackType = typename traits::callback_type;

  return CallbackType([instance, member_func](auto&&... args) {
    return (instance->*member_func)(std::forward<decltype(args)>(args)...);
  });
}

class EventBus {
  public:
  EventBus() = default;
  ~EventBus() = default;

  template <typename CallbackType>
  void subscribe(int32_t event_type, CallbackType callback) {
    auto type_it = m_cb_infos.find(event_type);
    if (type_it == m_cb_infos.end()) {
      m_cb_infos.insert({event_type, std::type_index(typeid(CallbackType))});
    } else {
      if (type_it->second != std::type_index(typeid(CallbackType))) {
        throw std::runtime_error("Expected same signature for " + std::to_string(event_type));
      }
    }
    auto& callback_list = get_all_callbacks<CallbackType>(event_type);
    callback_list.push_back(callback);
  }

  template <typename CallbackType>
  std::vector<CallbackType>& get_all_callbacks(int32_t event_type) {
    if (m_cbs.find(event_type) == m_cbs.end()) {
      m_cbs[event_type] = std::make_any<std::vector<CallbackType>>();
    }
    return std::any_cast<std::vector<CallbackType>&>(m_cbs[event_type]);
  }

  template <typename CallbackType, typename... Args>
  void fire_event(int32_t event_type, Args&&... args) {
    auto it = m_cbs.find(event_type);
    if (it != m_cbs.end()) {
      auto& callback_list = std::any_cast<std::vector<CallbackType>&>(it->second);
      for (const auto& callback : callback_list) {
        callback(std::forward<Args>(args)...);
      }
    }
  }

  private:
  std::unordered_map<int32_t, std::any> m_cbs;
  std::unordered_map<int32_t, std::type_index> m_cb_infos;
};
}  // namespace zerg