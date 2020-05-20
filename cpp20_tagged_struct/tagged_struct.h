#pragma once
#include <algorithm>
#include <cstdint>
#include <string_view>

template <std::size_t N>
struct fixed_string {
  constexpr fixed_string(const char (&foo)[N + 1]) {
    std::copy_n(foo, N + 1, data);
  }
  constexpr fixed_string(const fixed_string&) = default;
  constexpr fixed_string(std::string_view s) {
    std::copy_n(s.data(), N, data);
  };
  auto operator<=>(const fixed_string&) const = default;
  char data[N + 1] = {};
  constexpr std::string_view sv() const {
    return std::string_view(&data[0], N);
  }
  constexpr auto size() const { return N; }
  constexpr auto operator[](std::size_t i) const { return data[i]; }
};

template <std::size_t N>
fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

template <std::size_t N>
fixed_string(fixed_string<N>) -> fixed_string<N>;

template <typename T>
constexpr auto default_init = []() { T{}; };

template <typename Tag, typename T, auto Init = +default_init<T>>
struct member {
  T value = Init();
  member(const member&) = default;
  member& operator=(const member&) = default;
  member(member&&) = default;
  member& operator=(member&&) = default;
  template <typename OtherT, auto OtherInit>
  member(const member<Tag, OtherT, OtherInit>& other) : value(other.value){};
  member(member<Tag, OtherT, OtherInit>&& other)
      : value(std::move(other.value)){};
  member& operator=(const member<Tag, OtherT, OtherInit>& other) {
    value = other.value;
    return *this;
  }
  member& operator=(member<Tag, OtherT, OtherInit>&& other) {
    value = std::move(other.value);
    return *this;
  };

  using tag_type = Tag;
  using value_type = T;
};

template <typename Tag, typename T>
auto make_member(T t) {
  return member<Tag, T>{std::move(t)};
}

template <typename... Members>
struct tagged_struct : Members... {};

template <typename... Members>
tagged_struct(Members&&...) -> tagged_struct<std::decay_t<Members>...>;

template <typename Tag, typename T, auto Init>
decltype(auto) get_impl(member<Tag, T, Init>& m) {
  return (m.value);
}

template <typename Tag, typename T, auto Init>
decltype(auto) get_impl(const member<Tag, T, Init>& m) {
  return (m.value);
}

template <typename Tag, typename T, auto Init>
decltype(auto) get_impl(member<Tag, T, Init>&& m) {
  return std::move(m.value);
}

template <typename Tag, typename T, auto Init>
decltype(auto) get_impl(const member<Tag, T, Init>&& m) {
  return std::move(m.value);
}

template <fixed_string fs>
struct tuple_tag {
  static constexpr decltype(fs) value = fs;
  template <typename T>
  auto operator=(T t) {
    return member<tuple_tag<fixed_string<fs.size()>(fs)>, T>{std::move(t)};
  }
};

template <fixed_string fs, typename S>
decltype(auto) get(S&& s) {
  return get_impl<tuple_tag<fixed_string<fs.size()>(fs)>>(std::forward<S>(s));
}

template <typename S, typename Tag>
decltype(auto) operator->*(S&& s, Tag) {
  return get_impl<Tag>(std::forward<S>(s));
}

namespace literals {
template <fixed_string fs>
auto operator""_tag() {
  return tuple_tag<fixed_string<fs.size()>(fs)>();
}
};  // namespace literals
