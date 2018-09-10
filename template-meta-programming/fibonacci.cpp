// Copyright 2018 Google LLC
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

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <type_traits>

namespace specialization {
template <int I>
struct fib {
  enum { value = fib<I - 1>::value + fib<I - 2>::value };
};

template <>
struct fib<0> {
  enum { value = 1 };
};

template <>
struct fib<1> {
  enum { value = 1 };
};

}  // namespace specialization

namespace constexpr_function {
constexpr int fibonacci(int i) {
  return (i < 2) ? 1 : fibonacci(i - 1) + fibonacci(i - 2);
}
}  // namespace constexpr_function

namespace constexpr_function_iterative {
constexpr int fibonacci(int i) {
  if (i < 2) return 1;
  auto fib_prev = 1;
  auto fib_cur = 1;
  for (; i > 1; --i) {
    auto fib_next = fib_prev + fib_cur;
    fib_prev = fib_cur;
    fib_cur = fib_next;
  }
  return fib_cur;
}
}  // namespace constexpr_function_iterative

namespace templated {
template <typename T>
struct make_ptr {
  using type = T *;
};

template <typename T>
struct make_ptr<T *> {
  using type = T *;
};
}  // namespace templated

template <typename T>
struct t2t {
  using type = T;
};
namespace function {
template <typename T>
auto make_ptr(t2t<T>) {
  if constexpr (std::is_pointer_v<T>) {
    return t2t<T>{};
  } else {
    return t2t<T *>{};
  }
}
}  // namespace function

namespace variadic {
template <bool Done, int... I>
struct fib;

template <class Array>
constexpr auto reverse_array(Array ar) {
  Array ret{};
  auto begin = ar.rbegin();
  for (auto &a : ret) {
    a = *begin++;
  }
  return ret;
}

template <int... I>
struct fib<true, I...> {
  static constexpr std::array<int, sizeof...(I)> sequence =
      reverse_array(std::array<int, sizeof...(I)>{I...});
};

template <int fcur, int fprev, int... I>
    struct fib<false, fcur, fprev, I...>
    : fib < std::numeric_limits<int>::max() - fcur
                - fprev<fcur, fcur + fprev, fcur, fprev, I...> {};

constexpr auto fibonacci = fib<false, 1, 1>::sequence;

}  // namespace variadic

int main() {
  static_assert(specialization::fib<4>::value == 5);
  static_assert(constexpr_function::fibonacci(4) == 5);
  static_assert(constexpr_function_iterative::fibonacci(4) == 5);
  static_assert(variadic::fibonacci[4] == 5);
  std::copy(variadic::fibonacci.begin(), variadic::fibonacci.end(),
            std::ostream_iterator<int>(std::cout, "\n"));
  static_assert(std::is_same_v<templated::make_ptr<int>::type, int *>);
  static_assert(std::is_same_v<templated::make_ptr<int *>::type, int *>);
  static_assert(
      std::is_same_v<decltype(function::make_ptr(t2t<int>{}))::type, int *>);
  static_assert(
      std::is_same_v<decltype(function::make_ptr(t2t<int *>{}))::type, int *>);
}
