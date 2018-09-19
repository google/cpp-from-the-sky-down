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
// limitations under the License.

#include <cmath>
#include <ostream>
#include <string_view>
template <int M = 0, int K = 0, int S = 0>
struct unit {
  long double value;
};

using meter = unit<1, 0, 0>;
using kilogram = unit<0, 1, 0>;
using second = unit<0, 0, 1>;

template <int M, int K, int S>
unit<M, K, S> operator+(unit<M, K, S> a, unit<M, K, S> b) {
  return {a.value + b.value};
}

template <int M, int K, int S>
unit<M, K, S> operator-(unit<M, K, S> a, unit<M, K, S> b) {
  return {a.value - b.value};
}

template <int M1, int K1, int S1, int M2, int K2, int S2>
unit<M1 + M2, K1 + K2, S1 + S2> operator*(unit<M1, K1, S1> a,
                                          unit<M2, K2, S2> b) {
  return {a.value * b.value};
}

template <typename T, int M, int K, int S>
unit<M, K, S> operator*(T a, unit<M, K, S> b) {
  return {a * b.value};
}

template <int M1, int K1, int S1, int M2, int K2, int S2>
unit<M1 - M2, K1 - K2, S1 - S2> operator/(unit<M1, K1, S1> a,
                                          unit<M2, K2, S2> b) {
  return {a.value / b.value};
}

template <int N, int M, int K, int S>
unit<N * M, N * K, N * S> pow(unit<M, K, S> u) {
  static_assert(N >= 0, "pow only supports positive powers for now");

  if (N == 0) return {1.0};
  auto value = u.value;
  for (int i = 1; i < N; ++i) {
    value = value * value;
  }
  return {value};
}

template <int M, int K, int S>
unit<M / 2, K / 2, S / 2> sqrt(unit<M, K, S> u) {
  constexpr auto m = M / 2;
  constexpr auto k = K / 2;
  constexpr auto s = S / 2;
  static_assert(m * 2 == M && k * 2 == K && s * 2 == S,
                "Don't yet support fractional exponents");
  return {std::sqrt(u.value)};
}

using newton = unit<1, 1, -2>;

template <int V>
std::ostream& print_unit(std::ostream& os, std::string_view abbrev) {
  switch (V) {
    case 0:
      break;
    case 1:
      os << abbrev;
      break;
    default:
      os << abbrev << "^" << V;
  }
  return os;
}

template <int M, int K, int S>
std::ostream& operator<<(std::ostream& os, unit<M, K, S> u) {
  os << u.value;
  print_unit<M>(os, "m");
  print_unit<K>(os, "kg");
  print_unit<S>(os, "s");
  return os;
}

std::ostream& operator<<(std::ostream& os, newton n) {
  os << n.value << "N";
  return os;
}

using joule = unit<2, 1, -2>;
std::ostream& operator<<(std::ostream& os, joule n) {
  os << n.value << "J";
  return os;
}

meter operator""_m(long double v) { return {v}; }
kilogram operator""_kg(long double v) { return {v}; }
second operator""_s(long double v) { return {v}; }


