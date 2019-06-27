// Copyright 2019 Google LLC
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

#pragma once

#include "../simple_type_name/simple_type_name.h"
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace tagged_tuple {
template <typename Tag, typename T> struct member {
  T value;
  static constexpr std::string_view tag_name =
      simple_type_name::short_name<Tag>;
  using tag_type = Tag;
  using value_type = T;
};

template <typename Tag, typename T> auto make_member(T t) {
  return member<Tag, T>{std::move(t)};
}

template <typename... Members> struct ttuple : Members... {};

template <typename Tag, typename T> decltype(auto) get(member<Tag, T> &m) {
  return (m.value);
}

template <typename Tag, typename T>
decltype(auto) get(const member<Tag, T> &m) {
  return (m.value);
}

template <typename Tag, typename T> decltype(auto) get(member<Tag, T> &&m) {
  return std::move(m.value);
}

template <typename Tag, typename T>
decltype(auto) get(const member<Tag, T> &&m) {
  return std::move(m.value);
}

template <typename... Members>
constexpr auto tuple_size(const ttuple<Members...> &) {
  return sizeof...(Members);
}

template <typename... Members, typename... OtherMembers>
auto append(ttuple<Members...> t, OtherMembers... m) {
  return ttuple<Members..., OtherMembers...>{
      std::move(static_cast<Members &>(t))..., std::move(m)...};
}

template <typename... Members, typename... OtherMembers>
auto operator|(ttuple<Members...> t, ttuple<OtherMembers...> m) {
  return ttuple<Members..., OtherMembers...>{
      static_cast<Members &&>(t)..., static_cast<OtherMembers &&>(m)...};
}

template <typename... Members, typename F>
void for_each(const ttuple<Members...> &m, F f) {
  (f(static_cast<const Members &>(m)), ...);
}

template <typename... Members, typename F>
decltype(auto) apply(const ttuple<Members...> &m, F f) {
  return (f(static_cast<const Members &>(m)...));
}

namespace detail {
template <typename Tag, typename T>
std::true_type test_has_tag(const member<Tag, T> &);
template <typename Tag> std::false_type test_has_tag(...);

struct remove_member_tag {};
} // namespace detail

template <typename Tag, typename Tuple>
constexpr bool has_tag =
    decltype(detail::test_has_tag<Tag>(std::declval<Tuple>()))::value;

template <typename... Members> auto make_ttuple(Members... m) {
  return ttuple<Members...>{std::move(m)...};
}

template <typename Tag> auto remove_tag() {
  return make_member<Tag>(detail::remove_member_tag{});
}

template <typename T1, typename T2> auto merge(const T1 &, T2 t2) { return t2; }

template <typename... M1, typename... M2>
auto merge(ttuple<M1...> t1, ttuple<M2...> t2) {
  if constexpr (sizeof...(M1) == 0)
    return t2;
  else if constexpr (sizeof...(M2) == 0)
    return t1;
  else {
    using T1 = ttuple<M1...>;
    using T2 = ttuple<M2...>;

    auto transform_t1 = [&](auto &m) mutable {
      using Member = std::decay_t<decltype(m)>;
      using Tag = typename Member::tag_type;
      if constexpr (has_tag<Tag, T2>) {
        auto &v2 = get<Tag>(t2);
        using V2 = std::decay_t<decltype(v2)>;
        if constexpr (std::is_same_v<detail::remove_member_tag, V2>) {
          return ttuple<>{};
        } else {
          return make_ttuple(
              make_member<Tag>(merge(std::move(m.value), std::move(v2))));
        }
      } else {
        return make_ttuple(std::move(m));
      }
    };

    auto transform_t2 = [&](auto &member) mutable {
      using Member = std::decay_t<decltype(member)>;
      using Tag = typename Member::tag_type;
      if constexpr (std::is_same_v<detail::remove_member_tag,
                                   typename Member::value_type> ||
                    has_tag<Tag, T1>) {
        return ttuple<>{};
      } else {
        return make_ttuple(std::move(member));
      }
    };

    auto filtered_t1 = (... | transform_t1(static_cast<M1 &>(t1)));
    auto filtered_t2 = (... | transform_t2(static_cast<M2 &>(t2)));
    return filtered_t1 | filtered_t2;
  }
}

#include <ostream>

template <typename... Members>
std::ostream &operator<<(std::ostream &os, const ttuple<Members...> &t) {
  os << "{\n";
  auto output = [&](auto &v) mutable {
    os << v.tag_name << ": " << v.value << "\n";
  };
  for_each(t, output);

  os << "}\n";
  return os;
}

} // namespace tagged_tuple
