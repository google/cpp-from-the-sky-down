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

#include <initializer_list>
#include <type_traits>
#include <utility>
#include "../simple_type_name/simple_type_name.h"

namespace tagged_tuple {
template <typename Tag, typename T>
struct member {
  T value;
  static constexpr std::string_view tag_name =
      simple_type_name::short_name<Tag>;
  using tag_type = Tag;
  using value_type = T;
};

template <typename Tag, typename T>
auto make_member(T t) {
  return member<Tag, T>{std::move(t)};
}

template <typename... Members>
struct tagged_tuple : Members... {};

template <typename Tag, typename T>
decltype(auto) get(member<Tag, T>& m) {
  return (m.value);
}

template <typename Tag, typename T>
decltype(auto) get(const member<Tag, T>& m) {
  return (m.value);
}

template <typename Tag, typename T>
decltype(auto) get(member<Tag, T>&& m) {
  return std::move(m.value);
}

template <typename Tag, typename T>
decltype(auto) get(const member<Tag, T>&& m) {
  return std::move(m.value);
}

template <typename... Members>
constexpr auto tuple_size(const tagged_tuple<Members...>&) {
  return sizeof...(Members);
}

template <typename... Members, typename... OtherMembers>
auto append(tagged_tuple<Members...> t, OtherMembers... m) {
  return tagged_tuple<Members..., OtherMembers...>{
      std::move(static_cast<Members&>(t))..., std::move(m)...};
}

template <typename... Members, typename... OtherMembers>
auto operator|(tagged_tuple<Members...> t, tagged_tuple<OtherMembers...> m) {
  return tagged_tuple<Members..., OtherMembers...>{
      static_cast<Members&&>(t)..., static_cast<OtherMembers&&>(m)...};
}

template <typename... Members, typename F>
void for_each(const tagged_tuple<Members...>& m, F f) {
  (f(static_cast<const Members&>(m)), ...);
}

template <typename... Members, typename F>
decltype(auto) apply(const tagged_tuple<Members...>& m, F f) {
  return (f(static_cast<const Members&>(m)...));
}

namespace detail {
template <typename Tag, typename T>
std::true_type test_has_tag(const member<Tag, T>&);
template <typename Tag>
std::false_type test_has_tag(...);
}  // namespace detail

template <typename Tag, typename Tuple>
constexpr bool has_tag =
    decltype(detail::test_has_tag<Tag>(std::declval<Tuple>()))::value;

template <typename... Members>
auto make_tagged_tuple(Members... m) {
  return tagged_tuple<Members...>{std::move(m)...};
}

struct remove_member_tag {};

template <typename Tag>
auto remove_tag() {
  return make_member<Tag>(remove_member_tag{});
}

template <typename T1, typename T2>
auto merge(const T1&, T2 t2) {
  return t2;
}

template <typename... M1, typename... M2>
auto merge(tagged_tuple<M1...> t1, tagged_tuple<M2...> t2) {
  using T1 = decltype(t1);
  using T2 = decltype(t2);

  auto empty_if_in2 = [&](auto& member) mutable {
    using Member = std::decay_t<decltype(member)>;
    using Tag = typename Member::tag_type;
    if constexpr (has_tag<Tag, T2>) {
      return tagged_tuple<>{};
    } else {
      return make_tagged_tuple(std::move(member));
    }
  };

  auto combined_if_in1 = [&](auto& member) mutable {
    using Member = std::decay_t<decltype(member)>;
    using Tag = typename Member::tag_type;
    if constexpr (std::is_same_v<remove_member_tag,
                                 typename Member::value_type>) {
      return tagged_tuple<>{};
    } else if constexpr (has_tag<Tag, T1>) {
      return make_tagged_tuple(make_member<Tag>(
          merge(std::move(get<Tag>(t1)), std::move(member.value))));
    } else {
      return make_tagged_tuple(std::move(member));
    }
  };

  auto filtered_t1 = (... | empty_if_in2(static_cast<M1&>(t1)));
  auto filtered_t2 = (... | combined_if_in1(static_cast<M2&>(t2)));
  return filtered_t1 | filtered_t2;
}

namespace detail {}

#include <ostream>

template <typename... Members>
std::ostream& operator<<(std::ostream& os, const tagged_tuple<Members...>& t) {
  auto output = [&](auto& v) mutable {
    os << v.tag_name << ": " << v.value << "\n";
  };
  for_each(t, output);
  return os;
}

}  // namespace tagged_tuple
