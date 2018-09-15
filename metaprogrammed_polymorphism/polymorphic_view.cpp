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

#include <memory>
#include <utility>

namespace polymorphic {
namespace detail {

template <typename T>
using ptr = T*;

template <typename T>
struct type {};

template <typename Signature>
struct vtable_entry;

template <typename Method, typename Return, typename... Parameters>
struct vtable_entry<Return(Method, Parameters...)> {
  template <typename T>
  vtable_entry(type<T>)
      : fun_ptr_(+[](void* t, Parameters... parameters) -> Return {
          return poly_extend(Method{}, *static_cast<T*>(t), parameters...);
        }) {}

  auto call(Method, void* t, Parameters... parameters) const {
    return fun_ptr_(t, parameters...);
  }

  static constexpr bool is_const = false;
  ptr<Return(void*, Parameters...)> fun_ptr_;
};

template <typename Method, typename Return, typename... Parameters>
struct vtable_entry<Return(Method, Parameters...) const> {
  template <typename T>
  vtable_entry(type<T>)
      : fun_ptr_(+[](const void* t, Parameters... parameters) -> Return {
          return poly_extend(Method{}, *static_cast<const T*>(t),
                             parameters...);
        }) {}

  auto call(Method, const void* t, Parameters... parameters) const {
    return fun_ptr_(t, parameters...);
  }

  static constexpr bool is_const = true;
  ptr<Return(const void*, Parameters...)> fun_ptr_;
};

template <typename... Signatures>
struct vtable : vtable_entry<Signatures>... {
  using vtable_entry<Signatures>::call...;

  static constexpr bool all_const = (vtable_entry<Signatures>::is_const && ...);

  template <typename Other>
  vtable(Other& other) : vtable_entry<Signatures>(other)... {}

  template <typename T>
  vtable(type<T> t) : vtable_entry<Signatures>(t)... {}

  template <typename Other>
  static auto subset_vtable(const Other& other) {
    static const vtable vt(other);
    return &vt;
  }
};

template <typename T, typename VTable>
inline const auto type_specific_vtable = VTable{type<std::decay_t<T>>{}};

template <typename T>
std::false_type is_vtable(const T*);

template <typename... Signatures>
std::true_type is_vtable(const vtable<Signatures...>*);

template <typename T, typename = std::void_t<>>
struct is_polymorphic : std::false_type {};

template <typename T>
struct is_polymorphic<
    T, std::void_t<decltype(std::declval<const T&>().get_vptr())>>
    : decltype(is_vtable(std::declval<const T&>().get_vptr())) {};

}  // namespace detail

template <typename... Signatures>
class view {
  using vtable_t = detail::vtable<Signatures...>;
  const vtable_t* vptr_ = nullptr;

  std::conditional_t<vtable_t::all_const, const void*, void*> t_;

 public:
  template <typename T, typename = std::enable_if_t<
                            !detail::is_polymorphic<std::decay_t<T>>::value>>
  view(T& t) : vptr_(&detail::type_specific_vtable<T, vtable_t>), t_(&t) {}

  template <typename Poly, typename = std::enable_if_t<detail::is_polymorphic<
                               std::decay_t<Poly>>::value>>
  view(const Poly& other)
      : vptr_(vtable_t::subset_vtable(*other.get_vptr())),
        t_(other.get_ptr()){};

  explicit operator bool() const { return t_ != nullptr; }

  auto get_ptr() const { return t_; }

  auto get_vptr() const { return vptr_; }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return vptr_->call(Method{}, t_, std::forward<Parameters>(parameters)...);
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
      const detail::vtable<Signatures...,
                           detail::object_ptr(detail::clone) const>;
  vtable_t* vptr_ = nullptr;
  detail::object_ptr t_;

 public:
  auto get_vptr() const { return vptr_; }
  template <typename T, typename = std::enable_if_t<
                            !detail::is_polymorphic<std::decay_t<T>>::value>>
  object(T t)
      : vptr_(&detail::type_specific_vtable<T, vtable_t>),
        t_(detail::make_object_ptr(std::move(t))) {}

  template <typename Poly, typename = std::enable_if_t<detail::is_polymorphic<
                               std::decay_t<Poly>>::value>>
  object(const Poly& other)
      : vptr_(vtable_t::subset_vtable(*other.get_vptr())),
        t_(other.template call<detail::clone>()) {}

  object(const object& other)
      : vptr_{other.get_vptr()}, t_(other.call<detail::clone>()) {}

  object(object&&) = default;

  object& operator=(object&&) = default;

  object& operator=(const object& other) { (*this) = view(other); }

  explicit operator bool() const { return t_ != nullptr; }

  auto get_ptr() const { return t_.get(); }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) {
    return vptr_->call(Method{}, t_.get(),
                       std::forward<Parameters>(parameters)...);
  }

  template <typename Method, typename... Parameters>
  decltype(auto) call(Parameters&&... parameters) const {
    return vptr_->call(Method{}, t_.get(),
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

  using poly = polymorphic::view<void(draw) const, void(another)>;
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

  polymorphic::view<void(draw) const> p3{p2};

  p3.call<draw>();
}