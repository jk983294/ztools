#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace zerg {
template <typename TObject>
struct ObjectRegistry {
  using CreatorFunc = std::function<TObject*()>;

  std::unordered_map<std::string, CreatorFunc> ops;

  static ObjectRegistry& instance() {
    static ObjectRegistry reg;
    return reg;
  }

  static void register_op(const std::string& name, CreatorFunc creator) {
    auto& reg = instance();
    if (reg.ops.count(name)) throw std::runtime_error("duplicate op registration: " + name);
    reg.ops[name] = std::move(creator);
  }

  template <typename TDerived>
  static bool register_op(const std::string& name) {
    register_op(name, []() -> TObject* { return new TDerived; });
    return true;
  }
};
}

#define REGISTER_OP(BaseType, Name, ...)                                                           \
  namespace {                                                                                      \
  static const bool _reg_##Name = zerg::ObjectRegistry<BaseType>::register_op<__VA_ARGS__>(#Name); \
  }