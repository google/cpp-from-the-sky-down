#include <algorithm>
#include <cstddef>
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
struct member {
  constexpr static auto tag() { return Tag; }
  using element_type = T;
  T value;
};

template <typename... Members>
struct meta_struct : Members... {};


template<fixed_string tag, typename T>
decltype(auto) get_impl(member<tag, T>& m) {
  return (m.value);
}

template<fixed_string tag, typename T>
decltype(auto) get_impl(const member<tag, T>& m) {
  return (m.value);
}

template<fixed_string tag, typename T>
decltype(auto) get_impl(member<tag, T>&& m) {
  return std::move(m.value);
}

template<fixed_string tag, typename MetaStruct>
decltype(auto) get(MetaStruct&& s) {
  return get_impl<tag>(std::forward<MetaStruct>(s));
}

#include <iostream>
#include <string>

int main() {
  using Person = meta_struct<      //
      member<"id", int>,           //
      member<"name", std::string>  //
      >;

  Person p;
  get<"id">(p) = 1;
  get<"name">(p) = "John";

  std::cout << get<"id">(p) << " " << get<"name">(p) << "\n";


}
