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

#include <utility>
#include <initializer_list>
#include <type_traits>

namespace tagged_tuple {
template <typename Tag, typename T>
struct member {
  T value;
  using tag_type = Tag;
  using value_type = T;
};

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

}  // namespace tagged_tuple



