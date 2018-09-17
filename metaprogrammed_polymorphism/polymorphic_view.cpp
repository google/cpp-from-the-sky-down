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

#include <memory>
#include <utility>

namespace polymorphic {
namespace detail {

template <typename T>
using ptr = T*;

template <typename T>
struct type {};

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

  static decltype(auto) call(const vtable_fun* vt, const size_t* map, Method,
                             void* t, Parameters... parameters) {
    return reinterpret_cast<fun_ptr>(vt[map[I]])(t, parameters...);
  }

  static auto get_index(type<Return(Method, Parameters...)>) { return I; }

  static constexpr bool is_const = false;
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

  static decltype(auto) call(const vtable_fun* vt, const size_t* map, Method,
                             const void* t, Parameters... parameters) {
    return reinterpret_cast<fun_ptr>(vt[map[I]])(t, parameters...);
  }
  static auto get_index(type<Return(Method, Parameters...)const>) { return I; }

  static constexpr bool is_const = true;
  using fun_ptr = ptr<Return(const void*, Parameters...)>;
};

template <typename Sequence, typename... Signatures>
struct vtable;

template <size_t... I, typename... Signatures>
struct vtable<std::index_sequence<I...>, Signatures...>
    : vtable_entry<I, Signatures>... {
  using vtable_entry<I, Signatures>::call...;
  using vtable_entry<I, Signatures>::get_entry...;
  using vtable_entry<I, Signatures>::get_index...;

  static constexpr bool all_const =
      (vtable_entry<I, Signatures>::is_const && ...);

  template <typename T>
  static auto get_vtable(type<T> t) {
    static const vtable_fun vt[] = {
        vtable_entry<I, Signatures>::get_entry(t)...};
    return &vt[0];
  }

  static auto get_map() {
    static const size_t map[] = {I...};
    return &map[0];
  }
};

  template <typename Sequence, typename... Signatures,typename OtherSequence, typename... OtherSignatures>
  static auto subset_vtable(vtable<Sequence, Signatures...>,vtable<OtherSequence, OtherSignatures...> other) {
    static const size_t map[] = {other.get_index(type<Signatures>{})...};
    return &map[0];
  }

template <typename T>
std::false_type is_vtable(const T*);

std::true_type is_vtable(const vtable_fun*);

template <typename T, typename = std::void_t<>>
struct is_polymorphic : std::false_type {};

template <typename T>
struct is_polymorphic<
    T, std::void_t<decltype(std::declval<const T&>().get_vptr())>>
    : decltype(is_vtable(std::declval<const T&>().get_vptr())) {};

}  // namespace detail

template <typename... Signatures>
class view {
  using vtable_t =
      detail::vtable<std::make_index_sequence<sizeof...(Signatures)>,
                     Signatures...>;
  const detail::vtable_fun* vptr_ = nullptr;
  const size_t* map_ = nullptr;

  std::conditional_t<vtable_t::all_const, const void*, void*> t_;

 public:
  template <typename T, typename = std::enable_if_t<
                            !detail::is_polymorphic<std::decay_t<T>>::value>>
  view(T& t)
      : vptr_(vtable_t::get_vtable(detail::type<std::decay_t<T>>{})),
        map_(vtable_t::get_map()),
        t_(&t) {}

  template <typename Poly, typename = std::enable_if_t<detail::is_polymorphic<
                               std::decay_t<Poly>>::value>>
  view(const Poly& other)
      : vptr_(other.get_vptr()),
       map_(detail::subset_vtable(get_vtable(),other.get_vtable())),
        t_(other.get_ptr()){};

  explicit operator bool() const { return t_ != nullptr; }

  auto get_ptr() const { return t_; }

  auto get_vptr() const { return vptr_; }

  auto get_vtable() const{ return vtable_t{}; }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return vtable_t::call(vptr_, map_, Method{}, t_,
                          std::forward<Parameters>(parameters)...);
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
      const detail::vtable<std::make_index_sequence<sizeof...(Signatures)+1>,
                           Signatures...,
                           detail::object_ptr(detail::clone) const>;
  const detail::vtable_fun* vptr_ = nullptr;
  const size_t* map_ = nullptr;
  detail::object_ptr t_;

 public:
  auto get_vptr() const { return vptr_; }
  auto get_vtable() const { return vtable_t{}; }
  template <typename T, typename = std::enable_if_t<
                            !detail::is_polymorphic<std::decay_t<T>>::value>>
  object(T t)
      : vptr_(vtable_t::get_vtable(detail::type<T>{})),
       map_(vtable_t::get_map()),
        t_(detail::make_object_ptr(std::move(t))) {}

  template <typename Poly, typename = std::enable_if_t<detail::is_polymorphic<
                               std::decay_t<Poly>>::value>>
  object(const Poly& other)
      : vptr_(other.get_vptr()),
       map_(detail::subset_vtable(get_vtable(),other.get_vtable())),
        t_(other.template call<detail::clone>()) {}

  object(const object& other)
      : vptr_{other.get_vptr()},map_(other.map_), t_(other.call<detail::clone>()) {}

  object(object&&) = default;

  object& operator=(object&&) = default;

  object& operator=(const object& other) {
    (*this) = view(other); }

  explicit operator bool() const {
    return t_ != nullptr; }

  auto get_ptr() const {
    return t_.get(); }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) {
    return vtable_t::call(vptr_, map_, Method{}, t_.get(),
                          std::forward<Parameters>(parameters)...);
  }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return vtable_t::call(vptr_, map_, Method{}, t_.get(),
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

void call_draw(polymorphic::view<void(draw) const> drawable) {
  std::cout << "Calling draw: ";
  drawable.call<draw>();
}

int main() {
  int i = 5;

  using poly = polymorphic::view<void(another),void(draw) const >;
  using poly_object = polymorphic::object<void(draw) const, void(another)>;
  poly_object po{i};
  auto po1 = po;
  polymorphic::object<void(draw) const> po2 = po;
  call_draw(i);
  call_draw(po);
  call_draw(po2);
  po.call<another>();
  po1.call<draw>();
  po.call<draw>();

  poly a{po};

  call_draw(a);
  a.call<draw>();

  using poly_const = polymorphic::view<void(draw) const>;
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

  polymorphic::view<void(draw) const> p3{a};

  p3.call<draw>();
  p2.call<draw>();
}