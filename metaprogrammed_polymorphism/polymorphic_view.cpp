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

template <typename T>
struct type {};

template <typename T, typename Return, typename Method, typename... Parameters>
constexpr auto get_vtable_function(type<Return(Method, Parameters...)>) {
  return +[](Method method, void* t, Parameters... parameters) -> Return {
    return poly_extend(method, *static_cast<T*>(t), parameters...);
  };
}

template <typename T, typename Return, typename Method, typename... Parameters>
constexpr auto get_vtable_function(type<Return(Method, Parameters...) const>) {
  return +[](Method method, const void* t, Parameters... parameters) -> Return {
    return poly_extend(method, *static_cast<const T*>(t), parameters...);
  };
}

using vtable_entry = ptr<void()>;

template <typename T, typename... Signatures>
inline const vtable_entry vtable[] = {reinterpret_cast<ptr<void()>>(
    get_vtable_function<T>(type<Signatures>{}))...};

template <size_t Index, typename Return, typename Method,
          typename... Parameters>
constexpr auto get_vtable_caller(type<Return(Method, Parameters...)>) {
  return [](Method method, const vtable_entry* table, void* t,
            Parameters... parameters) -> Return {
    return reinterpret_cast<ptr<Return(Method, void*, Parameters...)>>(
        table[Index])(method, t, parameters...);
  };
}

template <size_t Index, typename Return, typename Method,
          typename... Parameters>
constexpr auto get_vtable_caller(type<Return(Method, Parameters...) const>) {
  return [](Method method, const vtable_entry* table, const void* t,
            Parameters... parameters) -> Return {
    return reinterpret_cast<ptr<Return(Method, const void*, Parameters...)>>(
        table[Index])(method, t, parameters...);
  };
}

template <size_t Index, typename TypedSignature>
constexpr auto get_vtable_entry(TypedSignature) {
  return [](TypedSignature, const vtable_entry* table) { return table[Index]; };
}

template <typename... T>
struct overload : T... {
  using T::operator()...;
};

template <typename... F>
overload(F... f)->overload<F...>;

template <typename Return, typename Method, typename... Parameters>
constexpr bool is_const(type<Return(Method, Parameters...) const>) {
  return true;
};

template <typename Return, typename Method, typename... Parameters>
constexpr bool is_const(type<Return(Method, Parameters...)>) {
  return false;
};

template <typename... Signatures>
constexpr bool all_signatures_const = (is_const(type<Signatures>()) && ...);

template <size_t... I, typename... TypedSignatures>
constexpr auto make_vtable_callers(std::index_sequence<I...>,
                                   TypedSignatures...) {
  return overload{get_vtable_caller<I>(TypedSignatures{})...};
}

template <size_t... I, typename... TypedSignatures>
constexpr auto make_vtable_getter(std::index_sequence<I...>,
                                  TypedSignatures...) {
  return overload{get_vtable_entry<I>(TypedSignatures{})...};
}

template <typename... Signatures>
constexpr auto vtable_callers = make_vtable_callers(
    std::make_index_sequence<sizeof...(Signatures)>{}, type<Signatures>{}...);

template <typename... Signatures>
constexpr auto vtable_getters = make_vtable_getter(
    std::make_index_sequence<sizeof...(Signatures)>{}, type<Signatures>{}...);

template <typename... Signatures>
struct polymorphic_caller_implementation {
  static constexpr bool all_const = all_signatures_const<Signatures...>;

  const vtable_entry* vptr_ = nullptr;
  polymorphic_caller_implementation(const vtable_entry* vptr) : vptr_{vptr} {}

  friend struct polymorphic_caller_implementation;

  template <typename Method, typename Ptr, typename... Parameters>
  decltype(auto) call_vtable(Method method, Ptr p,
                             Parameters&&... parameters) const {
    return vtable_callers<Signatures...>(
        method, vptr_, p, std::forward<Parameters>(parameters)...);
  }

  template <typename... OtherSignatures>
  static const vtable_entry* get_vtable(
      const polymorphic_caller_implementation<OtherSignatures...>& other) {
    static const vtable_entry vtable[] = {
        vtable_getters<OtherSignatures...>(type<Signatures>(), other.vptr_)...};
    return vtable;
  }

  template <typename... OtherSignatures>
  polymorphic_caller_implementation(
      const polymorphic_caller_implementation<OtherSignatures...>& other)
      : vptr_(get_vtable(other)) {}
};

}  // namespace detail
template <typename... Signatures>
class polymorphic_view;

template <typename... Signatures>
class polymorphic_object;

template <typename T>
struct is_polymorphic {
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

template <typename T>
auto get_ptr(T* t) {
  return t;
}

template <typename T>
auto get_ptr(T& t) {
  return t.get();
}

}  // namespace detail

template <typename... Signatures>
class polymorphic_object {
  detail::polymorphic_caller_implementation<Signatures...,
                                            object_ptr(detail::clone) const>
      pci_;
  object_ptr t_;

  auto& get_pci() const { return pci_; }
  template <typename IntPack, typename... OtherSignatures>
  friend struct polymorphic_caller_implementation;

  template <typename... OtherSignatures>
  friend class polymorphic_view;

 public:
  template <typename T, typename = std::enable_if_t<
                            !is_polymorphic<std::decay_t<T>>::value>>
  explicit polymorphic_object(T& t)
      : pci_{detail::vtable<std::decay_t<T>, Signatures...,
                            object_ptr(detail::clone)>},
        t_(make_object_ptr(t)) {}

  polymorphic_object(const polymorphic_object& other)
      : pci_{other.pci_.vptr_}, t_(other.call<detail::clone>()) {}

  polymorphic_object(polymorphic_object&&) = default;

  polymorphic_object& operator=(polymorphic_object&&) = default;

  polymorphic_object& operator=(const polymorphic_object& other) {
    (*this) = polymorphic_view(other);
  }

  explicit operator bool() const { return t_ != nullptr; }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) {
    return pci_.call_vtable(Method{}, t_.get(),
                            std::forward<Parameters>(parameters)...);
  }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return pci_.call_vtable(Method{}, t_.get(),
                            std::forward<Parameters>(parameters)...);
  }
};

template <typename... Signatures>
class polymorphic_view {
  detail::polymorphic_caller_implementation<Signatures...> pci_;

  const auto& get_pci() const { return pci_; }
  std::conditional_t<decltype(pci_)::all_const, const void*, void*> t_ =
      nullptr;

  using pci = detail::polymorphic_caller_implementation<
      std::make_index_sequence<sizeof...(Signatures)>, Signatures...>;

  template <typename... OtherSignatures>
  friend class polymorphic_view;

  template <typename IntPack, typename... OtherSignatures>
  friend struct polymorphic_caller_implementation;

 public:
  template <typename T, typename = std::enable_if_t<
                            !is_polymorphic<std::decay_t<T>>::value>>
  explicit polymorphic_view(T& t)
      : pci_(detail::vtable<std::decay_t<T>, Signatures...>), t_(&t) {}

  template <typename Poly, typename = std::enable_if_t<
                               is_polymorphic<std::decay_t<Poly>>::value>>
  explicit polymorphic_view(const Poly& other)
      : pci_(other.get_pci()), t_(detail::get_ptr(other.t_)){};

  explicit operator bool() const { return t_ != nullptr; }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return pci_.call_vtable(Method{}, t_,
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