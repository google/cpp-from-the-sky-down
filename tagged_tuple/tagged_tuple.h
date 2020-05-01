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
#include <ostream>
#include <type_traits>
#include <utility>

#include "../simple_type_name/simple_type_name.h"

namespace skydown {
template <typename Tag, typename T>
struct member {
  T value;
  static constexpr std::string_view tag_name = skydown::short_type_name<Tag>;
  using tag_type = Tag;
  using value_type = T;
};

template <typename Tag, typename T>
auto make_member(T t) {
  return member<Tag, T>{std::move(t)};
}

template <typename... Members>
struct tagged_tuple : Members... {};

template <typename... Members>
tagged_tuple(Members...) -> tagged_tuple<Members...>;

namespace detail {
template <typename Tag, typename T>
T element_type_helper(member<Tag, T> m);
}

template <typename Tag, typename TTuple>
using element_type_t =
    decltype(detail::element_type_helper<Tag>(std::declval<TTuple>()));

template <typename Tag, typename T>
decltype(auto) get(member<Tag, T> &m) {
  return (m.value);
}

template <typename Tag, typename T>
decltype(auto) get(const member<Tag, T> &m) {
  return (m.value);
}

template <typename Tag, typename T>
decltype(auto) get(member<Tag, T> &&m) {
  return std::move(m.value);
}

template <typename Tag, typename T>
decltype(auto) get(const member<Tag, T> &&m) {
  return std::move(m.value);
}

namespace skydown_tagged_tuple_internal {
template <typename Tag>
struct tag_function_type {
  template <typename T>
  decltype(auto) operator()(T &&t) const {
    return get<Tag>(std::forward<T>(t));
  }
  template <typename V>
  auto operator=(V &&v) const {
    return make_member<Tag>(std::forward<V>(v));
  }
};

template<typename T>
struct tuple_size_helper;

template<typename... Members>
struct tuple_size_helper<tagged_tuple<Members...>>:public std::integral_constant<int,sizeof...(Members)>{};

}  // namespace skydown_tagged_tuple_internal

template <typename Tag>
inline constexpr skydown_tagged_tuple_internal::tag_function_type<Tag> tag = {};

template <typename... Members>
constexpr auto tuple_size(const tagged_tuple<Members...> &) {
  return sizeof...(Members);
}


template<typename T>
inline constexpr auto tuple_size_v = skydown_tagged_tuple_internal::tuple_size_helper<std::decay_t<T>>::value;


template <typename... Members, typename... OtherMembers>
auto append(tagged_tuple<Members...> t, OtherMembers... m) {
  return tagged_tuple<Members..., OtherMembers...>{
      std::move(static_cast<Members &>(t))..., std::move(m)...};
}

template <typename... Members, typename... OtherMembers>
auto operator|(tagged_tuple<Members...> t, tagged_tuple<OtherMembers...> m) {
  return tagged_tuple<Members..., OtherMembers...>{
      static_cast<Members &&>(t)..., static_cast<OtherMembers &&>(m)...};
}

template <typename... Members, typename F>
void for_each(const tagged_tuple<Members...> &m, F f) {
  (f(static_cast<const Members &>(m)), ...);
}

template <typename... Members, typename F>
void for_each(tagged_tuple<Members...> &m, F f) {
  (f(static_cast<Members &>(m)), ...);
}

template <typename... Members, typename F>
decltype(auto) apply(const tagged_tuple<Members...> &m, F f) {
  return (f(static_cast<const Members &>(m)...));
}

template <typename... Members, typename F>
decltype(auto) apply(tagged_tuple<Members...> &m, F f) {
  return (f(static_cast<Members &>(m)...));
}

namespace detail {
template <typename Tag, typename T>
std::true_type test_has_tag(const member<Tag, T> &);
template <typename Tag>
std::false_type test_has_tag(...);

struct remove_member_tag {};
}  // namespace detail

template <typename Tag, typename Tuple>
constexpr bool has_tag =
    decltype(detail::test_has_tag<Tag>(std::declval<Tuple>()))::value;

template <typename Tag>
inline constexpr auto remove_tag = member<Tag,detail::remove_member_tag>{};

template <typename T1, typename T2>
auto merge(const T1 &, T2 t2) {
  return t2;
}

namespace skydown_merge_detail {
template <typename T>
struct empty {};

template <typename... Types>
struct typelist : empty<Types>... {};

template <typename T>
std::true_type typelist_has_impl(empty<T>);

template <typename T>
std::false_type typelist_has_impl(...);

template <typename List, typename T>
constexpr bool typelist_has = decltype(typelist_has_impl<T>(List{}))::value;

template <typename List, typename T, bool has>
struct append_maybe {
  using type = List;
};

template <typename... Types, typename T>
struct append_maybe<typelist<Types...>, T, false> {
  using type = typelist<Types..., T>;
};

template <typename List, typename T>
using append_maybe_t =
    typename append_maybe<List, T, typelist_has<List, T>>::type;

template <typename List, typename... T>
struct append_helper;

template <typename List, typename First, typename... Rest>
struct append_helper<List, First, Rest...> {
  using type =
      typename append_helper<append_maybe_t<List, First>, Rest...>::type;
};

template <typename List>
struct append_helper<List> {
  using type = List;
};

template <typename List, typename... T>
using append_helper_t = typename append_helper<List, T...>::type;

template <typename List1, typename List2>
struct typelist_union;

template <typename... Types1, typename... Types2>
struct typelist_union<typelist<Types1...>, typelist<Types2...>> {
  using type = append_helper_t<typelist<>, Types1..., Types2...>;
};

template <typename List1, typename List2>
using typelist_union_t = typename typelist_union<List1, List2>::type;

template <typename M>
using tag_t = typename M::tag_type;

template <typename TTuple>
struct tagged_tuple_to_typelist;

template <typename... M>
struct tagged_tuple_to_typelist<tagged_tuple<M...>> {
  using type = typelist<tag_t<M>...>;
};

template <typename TT>
using tagged_tuple_to_typelist_t = typename tagged_tuple_to_typelist<TT>::type;

template <typename Tag, typename TT1, typename TT2>
auto get_element(const TT1 &a, const TT2 &b) {
  using tl1 = tagged_tuple_to_typelist_t<TT1>;
  using tl2 = tagged_tuple_to_typelist_t<TT2>;

  static_assert(typelist_has<tl1, Tag> || typelist_has<tl2, Tag>);
  if constexpr (typelist_has<tl1, Tag> && typelist_has<tl2, Tag>) {
    if constexpr (std::is_same_v<std::decay_t<decltype(get<Tag>(b))>,
                                 detail::remove_member_tag>) {
      return tagged_tuple{};
    } else {
      return tagged_tuple{make_member<Tag>(merge(get<Tag>(a), get<Tag>(b)))};
    }
  } else if constexpr (typelist_has<tl1, Tag>) {
    return tagged_tuple{make_member<Tag>(get<Tag>(a))};
  } else if constexpr (typelist_has<tl2, Tag>) {
    return tagged_tuple{make_member<Tag>(get<Tag>(b))};
  }
}

template <typename TT1, typename TT2, typename... Tags>
auto merge_helper(const TT1 &a, const TT2 &b, typelist<Tags...>) {
  return (get_element<Tags>(a, b)|...);;
}

template <typename TT1, typename TT2>
auto merge_helper(const TT1 &a, const TT2 &b) {
  using tl1 = tagged_tuple_to_typelist_t<TT1>;
  using tl2 = tagged_tuple_to_typelist_t<TT2>;
  return merge_helper(a, b, typelist_union_t<tl1, tl2>{});
}

}  // namespace skydown_merge_detail

template <typename... M1, typename... M2>
auto merge(tagged_tuple<M1...> t1, tagged_tuple<M2...> t2) {
  return skydown_merge_detail::merge_helper(t1, t2);
}

namespace detail {
template <typename V>
auto has_ostream_op(V &v) -> decltype(std::declval<std::ostream &>() << v);

void has_ostream_op(...);

}  // namespace detail

template <typename... Members>
std::ostream &operator<<(std::ostream &os, const tagged_tuple<Members...> &t) {
  os << "{\n";
  auto output = [&](auto &v) mutable {
    if constexpr (!std::is_same_v<void,
                                  decltype(detail::has_ostream_op(v.value))>) {
      os << v.tag_name << " : " << v.value << "\n";
    } else {
      os << v.tag_name << " : "
         << skydown::short_type_name<decltype(v.value)> << "\n";
    }
  };
  for_each(t, output);

  os << "}\n";
  return os;
}

}  // namespace skydown
