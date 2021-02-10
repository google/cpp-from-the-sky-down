#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <utility>

namespace ftsd {

    namespace internal_tagged_struct{

template <std::size_t N>
struct fixed_string {
  constexpr fixed_string(const char (&foo)[N + 1]) {
    std::copy_n(foo, N + 1, data);
  }
  constexpr fixed_string(std::string_view s) {
    std::copy_n(s.data(), N, data);
  };
  auto operator<=>(const fixed_string&) const = default;
  char data[N + 1] = {};
  constexpr std::string_view sv() const {
    return std::string_view(&data[0], N);
  }
  constexpr std::size_t size() const { return N; }
  constexpr auto operator[](std::size_t i) const { return data[i]; }
};

template <std::size_t N>
fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

template <std::size_t N>
fixed_string(fixed_string<N>) -> fixed_string<N>;

template <typename Member>
constexpr auto member_tag() {
  return Member::fs.sv();
}

template <typename T>
struct type_to_type {
  using type = T;
};

template <typename... Members>
struct tagged_struct;

template <typename S, typename M>
struct chop_to_helper;

template <typename... Members, typename M>
struct chop_to_helper<tagged_struct<Members...>, M> {
  template <auto fs>
  static constexpr std::size_t find_index() {
    std::array<std::string_view, sizeof...(Members)> ar{
        member_tag<Members>()...};
    for (std::size_t i = 0; i < ar.size(); ++i) {
      if (ar[i] == fs.sv()) return i;
    }
  }

  static constexpr std::size_t index = find_index<M::fs>();
  decltype(std::make_index_sequence<index>()) sequence;
  using member_tuple = std::tuple<Members...>;
  template <std::size_t... I>
  static constexpr auto chop(std::index_sequence<I...>) {
    return type_to_type<
        tagged_struct<std::tuple_element_t<I, member_tuple>...>>{};
  }

  using type = typename decltype(chop(sequence))::type;
};

template <typename S, typename M>
using chopped = typename chop_to_helper<S, M>::type;

template <typename T>
constexpr auto default_init = []() { return T{}; };

struct dummy_conversion {};

template <typename Tag, typename T, auto Init = default_init<T>>
struct member_impl {
  T value;
  template <typename Self>
  member_impl(Self& self, dummy_conversion) requires requires {
    { Init() }
    ->std::convertible_to<T>;
  }
      : value(Init()) {
  }

      template <typename Self>
      member_impl(Self& self, dummy_conversion) requires requires {
        { Init() }
        ->std::same_as<void>;
      }
      {
        static_assert(!std::is_same_v<decltype(Init()), void>,
                      "Missing required argument.");
      }

      template <typename Self>
      member_impl(Self& self, dummy_conversion) requires requires {
        { Init(self) }
        ->std::convertible_to<T>;
      }
      : value(Init(self)) {
  }
      member_impl(T value) : value(std::move(value)) {}
      member_impl(const member_impl&) = default;
      member_impl& operator=(const member_impl&) = default;
      member_impl(member_impl&&) = default;
      member_impl& operator=(member_impl&&) = default;
      template <typename Self, typename OtherT, auto OtherInit>
      member_impl(Self& self, const member_impl<Tag, OtherT, OtherInit>& other)
          : value(other.value){};
      template <typename Self, typename OtherT, auto OtherInit>
      member_impl(Self& self, member_impl<Tag, OtherT, OtherInit>&& other)
          : value(std::move(other.value)){};
      template <typename OtherT, auto OtherInit>
      member_impl& operator=(const member_impl<Tag, OtherT, OtherInit>& other) {
        value = other.value;
        return *this;
      }
      template <typename OtherT, auto OtherInit>
      member_impl& operator=(member_impl<Tag, OtherT, OtherInit>&& other) {
        value = std::move(other.value);
        return *this;
      };

      using tag_type = Tag;
      using value_type = T;
};
template <fixed_string fs>
struct tuple_tag {
  static constexpr decltype(fs) value = fs;
  template <typename T>
  auto operator=(T t) const {
    return member_impl<tuple_tag<fixed_string<fs.size()>(fs)>, T>{std::move(t)};
  }
};

struct auto_;

template <typename Self, typename T, auto Init>
struct t_or_auto {
  using type = T;
};

template <typename Self, auto Init>
requires requires {
  {Init()};
}
struct t_or_auto<Self, auto_, Init> {
  using type = decltype(Init());
};

template <typename Self, typename T, auto Init>
requires requires {
  { Init(std::declval<Self&>()) }
  ->std::convertible_to<T>;
}
struct t_or_auto<Self, T, Init> {
  using type = decltype(Init(std::declval<Self&>()));
};

template <typename Self, auto Init>
requires requires {
  {Init(std::declval<Self&>())};
}
struct t_or_auto<Self, auto_, Init> {
  using type = decltype(Init(std::declval<Self&>()));
};

template <fixed_string Fs, typename T, auto Init = default_init<T>>
struct member {
  using type = T;
  using fs_type = decltype(Fs);
  static constexpr fs_type fs = Fs;
  static constexpr decltype(Init) init = Init;
};

template <typename Self, typename Member>
struct member_to_impl {
  using type =
      member_impl<tuple_tag<fixed_string<Member::fs.size()>(Member::fs)>,
                  typename t_or_auto<chopped<Self, Member>,
                                     typename Member::type, Member::init>::type,
                  Member::init>;
};

template <typename Self, typename Member>
using member_to_impl_t = typename member_to_impl<Self, Member>::type;

template <typename Tag, typename T>
auto make_member_impl(T t) {
  return member_impl<Tag, T>{std::move(t)};
}

template <typename... Members>
struct parameters : Members... {
  operator dummy_conversion() { return {}; }
};

template <typename... Members>
parameters(Members&&...) -> parameters<std::decay_t<Members>...>;

template <typename Self, typename... Members>
struct tagged_struct_base : member_to_impl_t<Self, Members>... {
  template <typename... Args>
  tagged_struct_base(Self& self, parameters<Args...> p)
      : member_to_impl_t<Self, Members>{self, p}... {}
};

template <typename Tag, typename T, auto Init>
decltype(auto) get_impl(member_impl<Tag, T, Init>& m) {
  return (m.value);
}

template <typename Tag, typename T, auto Init>
decltype(auto) get_impl(const member_impl<Tag, T, Init>& m) {
  return (m.value);
}

template <typename Tag, typename T, auto Init>
decltype(auto) get_impl(member_impl<Tag, T, Init>&& m) {
  return std::move(m.value);
}

template <typename Tag, typename T, auto Init>
decltype(auto) get_impl(const member_impl<Tag, T, Init>&& m) {
  return std::move(m.value);
}

template <fixed_string fs, typename S>
decltype(auto) get(S&& s) {
  return get_impl<tuple_tag<fixed_string<fs.size()>(fs)>>(std::forward<S>(s));
}

template <typename... Members>
struct tagged_struct
    : tagged_struct_base<tagged_struct<Members...>, Members...> {
  using super = tagged_struct_base<tagged_struct, Members...>;
  template <typename... Args>
  tagged_struct(Args&&... args)
      : super(*this, parameters{std::forward<Args>(args)...}) {}

  template <auto fs>
  auto& operator[](tuple_tag<fs>) {
    return get<fs>(*this);
  }
};


template<typename T>
struct member_impl_to_member;

template<auto fs, typename T, auto init>
struct member_impl_to_member<member_impl<tuple_tag<fs>,T,init>>{
    using type = member<fs,T,init>;
};

template<typename T>
using member_impl_to_member_t = typename member_impl_to_member<T>::type;

template <typename... Members>
tagged_struct(Members&&...) -> tagged_struct<member_impl_to_member_t<std::decay_t<Members>>...>;



template <fixed_string fs>
inline constexpr auto tag = tuple_tag<fixed_string<fs.size()>(fs)>{};

}

using internal_tagged_struct::get;
using internal_tagged_struct::tagged_struct;
using internal_tagged_struct::tag;
using internal_tagged_struct::member;
using internal_tagged_struct::auto_;

namespace literals {
template <internal_tagged_struct::fixed_string fs>  // placeholder type for deduction
constexpr auto operator""_tag() {
  return tag<fs>;
}
}  // namespace literals

}  // namespace ftsd
