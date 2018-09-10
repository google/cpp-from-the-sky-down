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
// See the License for the specific language governing permissions and
// limitations under the License.

#include <array>
#include <utility>

namespace polymorphic {
namespace detail {

template <typename T>
using ptr = T*;
template <typename T, typename Signature>
struct function_entry;

template <typename T, typename Return, typename Method, typename... Parameters>
struct function_entry<T, Return(Method, Parameters...)> {
  static Return poly_call(Method method, void* t, Parameters... parameters) {
    return poly_extend(method, *static_cast<T*>(t), parameters...);
  }
};

template <typename T, typename Return, typename Method, typename... Parameters>
struct function_entry<T, Return(Method, Parameters...) const> {
  static Return poly_call(Method method, const void* t,
                          Parameters... parameters) {
    return poly_extend(method, *static_cast<const T*>(t), parameters...);
  }
};

template <typename T>
struct type {};

template <typename T, typename... Signatures>
inline const auto vtable = std::array{
    reinterpret_cast<ptr<void()>>(function_entry<T, Signatures>::poly_call)...};

template <size_t Index, typename Signature>
struct vtable_caller;
template <size_t Index, typename Return, typename Method,
          typename... Parameters>
struct vtable_caller<Index, Return(Method, Parameters...)> {
  static Return call_vtable(Method method, ptr<const ptr<void()>> table,
                            void* t, Parameters... parameters) {
    return reinterpret_cast<ptr<Return(Method, void*, Parameters...)>>(
        table[Index])(method, t, parameters...);
  }
  constexpr static std::size_t get_index(type<Return(Method, Parameters...)>) {
    return Index;
  }

  constexpr static bool is_const = false;
};

template <size_t Index, typename Signature>
struct vtable_caller;
template <size_t Index, typename Return, typename Method,
          typename... Parameters>
struct vtable_caller<Index, Return(Method, Parameters...) const> {
  static Return call_vtable(Method method, ptr<const ptr<void()>> table,
                            const void* t, Parameters... parameters) {
    return reinterpret_cast<ptr<Return(Method, const void*, Parameters...)>>(
        table[Index])(method, t, parameters...);
  }
  constexpr static std::size_t get_index(
      type<Return(Method, Parameters...) const>) {
    return Index;
  }

  constexpr static bool is_const = true;
};

template <typename T>
using is_const_t = typename T::is_const;

template <typename... implementations>
struct overload_call_vtable : implementations... {
  using implementations::call_vtable...;
  using implementations::get_index...;
  static constexpr bool all_const = (implementations::is_const && ...);
};

template <typename IntPack, typename... Signatures>
struct polymorphic_caller_implementation;
template <size_t... I, typename... Signatures>
struct polymorphic_caller_implementation<std::index_sequence<I...>,
                                         Signatures...>
    : overload_call_vtable<vtable_caller<I, Signatures>...> {};

}  // namespace detail
template <typename... Signatures>
class polymorphic_view;

template <typename... Signatures>
class polymorphic_object;

template <typename T>
struct is_polymorphic{
  static constexpr bool value = false;
};

template <typename... Signatures>
struct is_polymorphic<polymorphic_view<Signatures...>> {
  static constexpr bool value = true;
};

template <typename... Signatures>
struct is_polymorphic<polymorphic_object<Signatures...>> {
  static constexpr bool value = true;
};

using object_ptr = std::unique_ptr<void, detail::ptr<void(void*)>>;

template <typename T>
object_ptr make_object_ptr(const T& t) {
  return object_ptr(new T(t), +[](void* v) { delete static_cast<T*>(v); });
}

namespace detail {
struct clone {};

template <typename T>
object_ptr poly_extend(clone, const T& t) {
  return make_object_ptr(t);
}

template<typename T>
auto get_ptr(T* t){return t;}

template<typename T>
auto get_ptr(T& t){return t.get();}


}  // namespace detail

template <typename... Signatures>
class polymorphic_object
    : private detail::polymorphic_caller_implementation<
          std::make_index_sequence<sizeof...(Signatures) + 1>, Signatures...,
          object_ptr(detail::clone) const> {
  const std::array<detail::ptr<void()>, sizeof...(Signatures) + 1>* vptr_ =
      nullptr;
  object_ptr t_;

  template <typename Signature>
  detail::ptr<void()> get_entry() const {
    return (*vptr_)[this->get_index(detail::type<Signature>{})];
  }

  template <typename... OtherSignatures>
  friend class polymorphic_view;

 public: template <typename T, typename = std::enable_if_t< !is_polymorphic<std::decay_t<T>>::value>>
  explicit polymorphic_object(T& t)
      : vptr_(&detail::vtable<std::decay_t<T>, Signatures...,
                              object_ptr(detail::clone)>),
        t_(make_object_ptr(t)) {}

  polymorphic_object(const polymorphic_object& other)
      : vptr_(other.vptr_), t_(other.call<detail::clone>()) {}

  polymorphic_object(polymorphic_object&&) = default;

  polymorphic_object& operator=(polymorphic_object&&) = default;

  polymorphic_object& operator=(const polymorphic_object& other) {
    (*this) = polymorphic_view(other);
  }

  explicit operator bool() const { return t_ != nullptr; }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) {
    return this->call_vtable(Method{}, vptr_->data(), t_.get(),
                             std::forward<Parameters>(parameters)...);
  }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return this->call_vtable(Method{}, vptr_->data(), t_.get(),
                             std::forward<Parameters>(parameters)...);
  }
};

template <typename... Signatures>
class polymorphic_view
    : private detail::polymorphic_caller_implementation<
          std::make_index_sequence<sizeof...(Signatures)>, Signatures...> {
  const std::array<detail::ptr<void()>, sizeof...(Signatures)>* vptr_ = nullptr;
  std::conditional_t<polymorphic_view::all_const, const void*, void*> t_ =
      nullptr;

  template <typename Poly>
  static auto get_vtable(const Poly& other) {
    static const std::array<detail::ptr<void()>, sizeof...(Signatures)> vtable{
        other.template get_entry<Signatures>()...};
    return &vtable;
  }

  template <typename Signature>
  detail::ptr<void()> get_entry() const {
    return (*vptr_)[this->get_index(detail::type<Signature>{})];
  }

  template <typename... OtherSignatures>
  friend class polymorphic_view;

 public:
  template <typename T, typename = std::enable_if_t<
                            !is_polymorphic<std::decay_t<T>>::value>>
  explicit polymorphic_view(T& t)
      : vptr_(&detail::vtable<std::decay_t<T>, Signatures...>), t_(&t) {}

  template <typename Poly, typename = std::enable_if_t<
                            is_polymorphic<std::decay_t<Poly>>::value>>
  explicit polymorphic_view(const Poly& other)
      : vptr_(get_vtable(other)), t_(detail::get_ptr(other.t_)) {}

  explicit operator bool() const { return t_ != nullptr; }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) {
    return this->call_vtable(Method{}, vptr_->data(), t_,
                             std::forward<Parameters>(parameters)...);
  }
};

}  // namespace polymorphic

#include <iostream>
#include <string>

struct draw {};
struct another {};

template <typename T>
void poly_extend(draw, const T& t) {
  std::cout << t << "\n";
}
template <typename T>
void poly_extend(another, T& t) {
  t = t + t;
}

int main() {
  int i = 5;

  using poly = polymorphic::polymorphic_view<void(draw) const, void(another)>;
  using poly_object =
      polymorphic::polymorphic_object<void(draw) const, void(another)>;
  poly_object po{i};
  auto po1 = po;
  po.call<another>();
  po1.call<draw>();
  po.call<draw>();

  poly a{po};
  a.call<draw>();

  using poly_const = polymorphic::polymorphic_view<void(draw) const>;
  const int ic = 7;
  poly_const pc1{ic};
  pc1.call<draw>();
  poly p1{i};
  p1.call<draw>();
  auto p2 = p1;
  p2.call<draw>();
  std::string s("hello world");
  p2 = poly{s};
  p2.call<another>();
  p2.call<draw>();

  polymorphic::polymorphic_view<void(draw) const> p3{p2};

  p3.call<draw>();
}