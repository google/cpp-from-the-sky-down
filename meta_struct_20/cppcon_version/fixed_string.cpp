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

#include <iostream>
template<fixed_string s>
void print(std::ostream& os) {
  static_assert(s == fixed_string("hello world"));
}

int main() { print<"hello world">(std::cout); }
