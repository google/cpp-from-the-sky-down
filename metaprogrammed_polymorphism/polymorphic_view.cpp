// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific

#include <array>
#include <utility>

namespace polymorphic {
namespace detail {
template <typename T, typename Signature>
struct function_entry;

template <typename T, typename Return, typename Method, typename... Parameters>
struct function_entry<T, Return(Method, Parameters...)> {
  static Return poly_call(Method method, void* t, Parameters... parameters) {
    return poly_extend(method, *static_cast<T*>(t), parameters...);
  }
};

template <typename T, typename... Signatures>
inline const auto vtable = std::array{
    reinterpret_cast<void (*)()>(function_entry<T, Signatures>::poly_call)...};

template <size_t Index, typename Signature>
struct vtable_caller;
template <size_t Index, typename Return, typename Method,
          typename... Parameters>
struct vtable_caller<Index, Return(Method, Parameters...)> {
  template <size_t Size>
  Return call_vtable(Method method, const std::array<void (*)(), Size>& table,
                     void* t, Parameters... parameters) {
    return reinterpret_cast<Return (*)(Method, void*, Parameters...)>(
        table[Index])(method, t, parameters...);
  }
};

template <typename... implementations>
struct overload_call_vtable : implementations... {
  using implementations::call_vtable...;
};

template <typename IntPack, typename... Signatures>
struct polymorphic_caller_implementation;
template <size_t... I, typename... Signatures>
struct polymorphic_caller_implementation<std::index_sequence<I...>,
                                         Signatures...>
    : overload_call_vtable<vtable_caller<I, Signatures>...> {};

}  // namespace detail

template <typename... Signatures>
class polymorphic_view
    : private detail::polymorphic_caller_implementation<
          std::make_index_sequence<sizeof...(Signatures)>, Signatures...> {
  const std::array<void (*)(), sizeof...(Signatures)>* vptr_ = nullptr;
  void* t_ = nullptr;

 public:
  template <typename T>
  explicit polymorphic_view(T& t)
      : vptr_(&detail::vtable<std::decay_t<T>, Signatures...>), t_(&t) {}

  explicit operator bool() const { return t != nullptr; }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&...) {
    return this->call_vtable(Method{}, *vptr_, t_,
                             std::forward<Parameters>(parameters)...);
  }
};

}  // namespace polymorphic

#include <iostream>
#include <string>

struct draw {};
struct another {};

template <typename T>
void poly_extend(draw, T& t) {
  std::cout << t << "\n";
}
template <typename T>
void poly_extend(another, T& t) {
  t = t + t;
}

int main() {
  int i = 5;
  using poly = polymorphic::polymorphic_view<void(draw), void(another)>;
  poly p1{i};
  p1.call<draw>();
  auto p2 = p1;
  p2.call<draw>();
  std::string s("hello world");
  p2 = poly{s};
  p2.call<another>();
  p2.call<draw>();
}
