#include <utility>

template <std::size_t I, typename T> struct holder { T t; };

template <typename Sequence, typename... Ts> struct tuple_impl;

template <std::size_t... I, typename... Ts>
struct tuple_impl<std::index_sequence<I...>, Ts...> : holder<I, Ts>... {};

template <typename... Ts>
using tuple = tuple_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

template <std::size_t I, typename T> T &get(holder<I, T> &h) { return h.t; }

template <typename T, std::size_t I> T &get(holder<I, T> &h) { return h.t; }

#include <iostream>
#include <string>


void test_tuple() {
  tuple<int, std::string> t{1, "hello"};
  std::cout << get<1>(t) << "\n";
  std::cout << get<0>(t) << "\n";

  get<int>(t) = 5;
  get<std::string>(t) = "world";

  std::cout << get<1>(t) << "\n";
  std::cout << get<0>(t) << "\n";
}

#include <variant>

template <typename... F> struct overload : F... { using F::operator()...; };

template <typename... F> overload(F...)->overload<F...>;

void test_overload() {
  std::variant<int, std::string> v;
  v = std::string("hello");

  std::visit(
      overload{[](int i) { std::cout << i << " int \n"; },
               [](const std::string &s) { std::cout << s << " string \n"; }},
      v);
}

int main() {
  test_tuple();
  test_overload();
}
