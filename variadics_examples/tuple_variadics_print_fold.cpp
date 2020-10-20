#include <cstdint>
#include <iostream>
#include <type_traits>
#include <utility>

using std::size_t;

template <size_t I, typename T>
struct Holder {
  T value;
};

template <typename T, size_t I>
decltype(auto) get(const Holder<I, T>& h) {
  return h.value;
}

template <typename T, size_t I>
decltype(auto) get(Holder<I, T>& h) {
  return h.value;
}

template <size_t I, typename T>
decltype(auto) get(const Holder<I, T>& h) {
  return h.value;
}

template <size_t I, typename T>
decltype(auto) get(Holder<I, T>& h) {
  return h.value;
}

template <typename Sequence, typename... Ts>
struct tuple_impl;

template <typename... Ts, size_t... Is>
struct tuple_impl<std::index_sequence<Is...>, Ts...> : Holder<Is, Ts>... {
  tuple_impl() = default;
  tuple_impl(Ts... ts) : Holder<Is, Ts>{std::move(ts)}... {}
};

template <typename... Ts>
struct tuple : tuple_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...> {
  using base = tuple_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
  using base::base;
};

template <typename T>
struct type2type {
  using type = T;
};

template <size_t I, typename T>
type2type<T> element_helper(Holder<I, T>& h);

template <typename Tuple, size_t I>
using tuple_element_t =
    typename decltype(element_helper<I>(std::declval<Tuple&>()))::type;

template <typename Tuple, typename Sequence>
struct tuple_select;

template <typename Tuple, size_t... Is>
struct tuple_select<Tuple, std::index_sequence<Is...>> {
  using type = tuple<tuple_element_t<Tuple, Is>...>;
};

template <typename Tuple, typename Sequence>
using tuple_select_t = typename tuple_select<Tuple, Sequence>::type;

template <size_t I, size_t Last>
constexpr auto index_reversed = Last - I;

template <typename Tuple>
struct tuple_size;

template <typename... Ts>
struct tuple_size<tuple<Ts...>> {
  static constexpr size_t value = sizeof...(Ts);
};

template <typename Tuple>
inline constexpr auto tuple_size_v = tuple_size<Tuple>::value;

template <typename F, typename Tuple, size_t... Is>
decltype(auto) apply_helper(Tuple& t, F& f, std::index_sequence<Is...>) {
  return f(get<Is>(t)...);
}

template <typename F, typename Tuple>
decltype(auto) apply(Tuple& t, F&& f) {
  return apply_helper(
      t, f, std::make_index_sequence<tuple_size_v<std::decay_t<Tuple>>>());
}

template <typename Tuple>
void print(std::ostream& os, const Tuple& t) {
  auto print_one = [&](auto& t) mutable { os << t << "\n"; };
  auto print_args = [&](auto&&... ts) mutable { (print_one(ts), ...); };
  apply(t, print_args);
}

int main() {
  tuple<int, double> t2{1, 3.5};
  print(std::cout, t2);
}