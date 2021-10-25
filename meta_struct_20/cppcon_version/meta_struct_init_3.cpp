#include <algorithm>
#include <cstddef>
#include <type_traits>

template <std::size_t N>
struct fixed_string {
  constexpr fixed_string(const char (&foo)[N + 1]) {
    std::copy_n(foo, N + 1, data);
  }
  auto operator<=>(const fixed_string&) const = default;
  char data[N + 1] = {};
};
template <std::size_t N>
fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

template <fixed_string Tag, typename T>
struct tag_and_value {
  T value;
};

template <typename... TagsAndValues>
struct parms : TagsAndValues... {};

template <typename... TagsAndValues>
parms(TagsAndValues...) -> parms<TagsAndValues...>;

template <fixed_string Tag>
struct arg_type {
  template <typename T>
  constexpr auto operator=(T t) const {
    return tag_and_value<Tag, T>{std::move(t)};
  }
};

template <fixed_string Tag>
inline constexpr auto arg = arg_type<Tag>{};

template <typename T>
struct default_init {
  constexpr default_init() = default;
  auto operator<=>(const default_init&) const = default;
  constexpr auto operator()() const {
    if constexpr (std::is_default_constructible_v<T>) {
      return T{};
    }
  }
};

template <fixed_string Tag, typename T, auto Init = default_init<T>()>
struct member {
  constexpr static auto tag() { return Tag; }
  constexpr static auto init() { return Init; }
  using element_type = T;
  T value;
  template <typename OtherT>
  constexpr member(tag_and_value<Tag, OtherT> tv)
      : value(std::move(tv.value)) {}

  constexpr member() : value(Init()) {}
  constexpr member(member&&) = default;
  constexpr member(const member&) = default;

  constexpr member& operator=(member&&) = default;
  constexpr member& operator=(const member&) = default;

  auto operator<=>(const member&) const = default;
};

template <typename... Members>
struct meta_struct_impl : Members... {
  template <typename Parms>
  constexpr meta_struct_impl(Parms p) : Members(std::move(p))... {}

  constexpr meta_struct_impl() = default;
  constexpr meta_struct_impl(meta_struct_impl&&) = default;
  constexpr meta_struct_impl(const meta_struct_impl&) = default;
  constexpr meta_struct_impl& operator=(meta_struct_impl&&) = default;
  constexpr meta_struct_impl& operator=(const meta_struct_impl&) = default;

  auto operator<=>(const meta_struct_impl&) const = default;
};

template <typename... Members>
struct meta_struct : meta_struct_impl<Members...> {
  using super = meta_struct_impl<Members...>;
  template <typename... TagsAndValues>
  constexpr meta_struct(TagsAndValues... tags_and_values)
      : super(parms(std::move(tags_and_values)...)) {}

  constexpr meta_struct() = default;
  constexpr meta_struct(meta_struct&&) = default;
  constexpr meta_struct(const meta_struct&) = default;
  constexpr meta_struct& operator=(meta_struct&&) = default;
  constexpr meta_struct& operator=(const meta_struct&) = default;

  auto operator<=>(const meta_struct&) const = default;
};

template <fixed_string tag, typename T, auto Init>
decltype(auto) get_impl(member<tag, T, Init>& m) {
  return (m.value);
}

template <fixed_string tag, typename T, auto Init>
decltype(auto) get_impl(const member<tag, T, Init>& m) {
  return (m.value);
}

template <fixed_string tag, typename T, auto Init>
decltype(auto) get_impl(member<tag, T, Init>&& m) {
  return std::move(m.value);
}

template <fixed_string tag, typename MetaStruct>
decltype(auto) get(MetaStruct&& s) {
  return get_impl<tag>(std::forward<MetaStruct>(s));
}

#include <iostream>
#include <string>

int main() {
  using Person = meta_struct<                             //
      member<"id", int>,                                  //
      member<"name", std::string, [] { return "John"; }>  //
      >;

  Person p;

  std::cout << get<"id">(p) << " " << get<"name">(p) << "\n";
}
