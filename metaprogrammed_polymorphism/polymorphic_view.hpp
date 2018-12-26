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
// limitations under the License.

#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

namespace polymorphic {
namespace detail {

template <typename T>
using ptr = T*;

template <typename T>
struct type {};

using index_type = std::uint8_t;

using vtable_fun = ptr<void()>;

template <size_t I, typename Signature>
struct vtable_entry;

template <size_t I, typename Method, typename Return, typename... Parameters>
struct vtable_entry<I, Return(Method, Parameters...)> {
  template <typename T>
  static auto get_entry(type<T>) {
    return reinterpret_cast<vtable_fun>(
        +[](void* t, Parameters... parameters) -> Return {
          return poly_extend(Method{}, *static_cast<T*>(t), parameters...);
        });
  }

  static decltype(auto) call_imp(const vtable_fun* vt,
                                 const index_type* permutation, Method, void* t,
                                 Parameters... parameters) {
    return reinterpret_cast<fun_ptr>(vt[permutation[I]])(t, parameters...);
  }

  static auto get_index(type<Return(Method, Parameters...)>) { return I; }

  using is_const =  std::false_type ;
  using fun_ptr = ptr<Return(void*, Parameters...)>;
};

template <size_t I, typename Method, typename Return, typename... Parameters>
struct vtable_entry<I, Return(Method, Parameters...) const> {
  template <typename T>
  static auto get_entry(type<T>) {
    return reinterpret_cast<vtable_fun>(
        +[](const void* t, Parameters... parameters) -> Return {
          return poly_extend(Method{}, *static_cast<const T*>(t),
                             parameters...);
        });
  }

  static decltype(auto) call_imp(const vtable_fun* vt,
                                 const index_type* permutation, Method,
                                 const void* t, Parameters... parameters) {
    return reinterpret_cast<fun_ptr>(vt[permutation[I]])(t, parameters...);
  }
  static auto get_index(type<Return(Method, Parameters...) const>) { return I; }

  using is_const = std::true_type ;
  using fun_ptr = ptr<Return(const void*, Parameters...)>;
};

template<typename T>
using is_const_t = typename T::is_const;

template <typename... entry>
struct entries : entry... {
  template <typename... T>
  entries(T&&... t) : entry(t...)... {}

  using entry::call_imp...;
  using entry::get_entry...;
  using entry::get_index...;

  static constexpr bool all_const() { return std::conjunction_v<is_const_t<entry>...>; }
};

template <typename Sequence, typename... Signatures>
struct vtable;

template <size_t... I, typename... Signatures>
struct vtable<std::index_sequence<I...>, Signatures...>
    : entries<vtable_entry<I, Signatures>...> {
  static_assert(sizeof...(Signatures) <=
                std::numeric_limits<index_type>::max());

  template <typename T>
  static auto get_vtable(type<T> t) {
    static const vtable_fun vt[] = {
        vtable_entry<I, Signatures>::get_entry(t)...};
    return &vt[0];
  }

  template <typename T>
  vtable(type<T> t) : vptr_(get_vtable(t)), permutation_{I...} {}

  const vtable_fun* vptr_;
  std::array<detail::index_type, sizeof...(Signatures)> permutation_;

  template <typename OtherSequence, typename... OtherSignatures>
  vtable(const vtable<OtherSequence, OtherSignatures...>& other)
      : vptr_(other.vptr_), permutation_{other.permutation_[other.get_index(type<Signatures>{})]...} {}


  template <typename VoidType, typename Method, typename... Parameters>
  decltype(auto) call(Method method, VoidType t,
                      Parameters&&... parameters) const {
    return vtable::call_imp(vptr_, permutation_.data(), method, t,
                            std::forward<Parameters>(parameters)...);
  }
};

template <typename T>
std::false_type is_vtable(const T*);

template <typename Sequence, typename... Signatures>
std::true_type is_vtable(const vtable<Sequence, Signatures...>&);

template <typename T, typename = std::void_t<>>
struct is_polymorphic : std::false_type {};

template <typename T>
struct is_polymorphic<
    T, std::void_t<decltype(std::declval<const T&>().get_vtable())>>
    : decltype(is_vtable(std::declval<const T&>().get_vtable())) {};

}  // namespace detail

template <typename... Signatures>
class view {
  using vtable_t =
      detail::vtable<std::make_index_sequence<sizeof...(Signatures)>,
                     Signatures...>;

  vtable_t vt_;
  std::conditional_t<vtable_t::all_const(), const void*, void*> t_;

 public:
  template <typename T, typename = std::enable_if_t<
                            !detail::is_polymorphic<std::decay_t<T>>::value>>
  view(T& t) : vt_(detail::type<std::decay_t<T>>{}), t_(&t) {}

  template <typename Poly, typename = std::enable_if_t<detail::is_polymorphic<
                               std::decay_t<Poly>>::value>>
  view(const Poly& other) : vt_(other.get_vtable()), t_(other.get_ptr()){};

  explicit operator bool() const { return t_ != nullptr; }

  const auto& get_vtable() const { return vt_; }

  auto get_ptr() const { return t_; }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return vt_.call(Method{}, t_, std::forward<Parameters>(parameters)...);
  }
};

namespace detail {
using object_ptr = std::unique_ptr<void, detail::ptr<void(void*)>>;

template <typename T>
object_ptr make_object_ptr(const T& t) {
  return object_ptr(new T(t), +[](void* v) { delete static_cast<T*>(v); });
}

struct clone {};

template <typename T>
object_ptr poly_extend(clone, const T& t) {
  return make_object_ptr(t);
}
}  // namespace detail

template <typename... Signatures>
class object {
  using vtable_t =
      const detail::vtable<std::make_index_sequence<sizeof...(Signatures) + 1>,
                           Signatures...,
                           detail::object_ptr(detail::clone) const>;

  vtable_t vt_;
  detail::object_ptr t_;

 public:
  const auto& get_vtable() const { return vt_; }
  template <typename T, typename = std::enable_if_t<
                            !detail::is_polymorphic<std::decay_t<T>>::value>>
  object(T t)
      : vt_(detail::type<T>{}), t_(detail::make_object_ptr(std::move(t))) {}

  template <typename Poly, typename = std::enable_if_t<detail::is_polymorphic<
                               std::decay_t<Poly>>::value>>
  object(const Poly& other)
      : vt_(other.get_vtable()), t_(other.template call<detail::clone>()) {}

  object(const object& other)
      : vt_{other.get_vtable()}, t_(other.call<detail::clone>()) {}

  object(object&&) = default;

  object& operator=(object&&) = default;

  object& operator=(const object& other) { (*this) = view(other); }

  explicit operator bool() const { return t_ != nullptr; }

  auto get_ptr() const { return t_.get(); }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) {
    return vt_.call(Method{}, t_.get(),
                    std::forward<Parameters>(parameters)...);
  }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return vt_.call(Method{}, t_.get(),
                    std::forward<Parameters>(parameters)...);
  }
};

}  // namespace polymorphic
