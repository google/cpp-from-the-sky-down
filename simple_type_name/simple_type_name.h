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

#include <cstdint>
#include <string_view>

namespace skydown {

namespace detail {

#if !defined(__clang__) && !defined(__GNUC__) && defined(_MSC_VER)
#define SIMPLE_TYPE_NAME_PRETTY_FUNCTION __FUNCSIG__
#else
#define SIMPLE_TYPE_NAME_PRETTY_FUNCTION __PRETTY_FUNCTION__

#endif

template <typename T>
constexpr std::string_view type_to_string_raw() {
  return SIMPLE_TYPE_NAME_PRETTY_FUNCTION;
}

constexpr std::string_view long_double_raw_string =
    type_to_string_raw<long double>();

constexpr std::string_view long_double_string = "long double";

constexpr std::size_t begin_type_name =
    long_double_raw_string.find(long_double_string);
static_assert(begin_type_name != std::string_view::npos);

constexpr std::size_t end_type_name =
    begin_type_name + long_double_string.size();
static_assert(begin_type_name != std::string_view::npos);

constexpr std::size_t suffix_type_name_size =
    long_double_raw_string.size() - end_type_name;

template <typename T>
constexpr std::string_view long_name() {
  std::string_view raw_name = type_to_string_raw<T>();
  auto size = raw_name.size();
  raw_name.remove_prefix(begin_type_name);
  raw_name.remove_suffix(suffix_type_name_size);
  std::string_view struct_name("struct ");
  std::string_view class_name("class ");

  if (raw_name.substr(0, struct_name.size()) == struct_name)
    raw_name.remove_prefix(struct_name.size());
  if (raw_name.substr(0, class_name.size()) == class_name)
    raw_name.remove_prefix(class_name.size());

  while (!raw_name.empty() && raw_name.back() == ' ') {
    raw_name.remove_suffix(1);
  }
  return raw_name;
}

#undef SIMPLE_TYPE_NAME_PRETTY_FUNCTION

template <typename T>
constexpr std::string_view short_name() {
  auto raw_str = long_name<T>();
  int last = -1;
  int count = 0;
  for (std::size_t pos = 0; pos < raw_str.size(); ++pos) {
    auto& c = raw_str[pos];
    if (c == '<') ++count;
    if (c == '>') --count;
    if (c == ':' && count == 0) last = pos;
  }
  if (last != -1) {
    raw_str.remove_prefix(last + 1);
  }
  return raw_str;
}

}  // namespace detail

template <typename T>
inline constexpr std::string_view short_type_name = detail::short_name<T>();

template <typename T>
inline constexpr std::string_view long_type_name = detail::long_name<T>();

namespace simple_type_name_testing {
static_assert(long_type_name<long double> == detail::long_double_string);
static_assert(short_type_name<long double> == detail::long_double_string);

struct MyClass;

template <typename T>
class TemplateTester;

static_assert(skydown::short_type_name<int> == "int");
static_assert(skydown::short_type_name<TemplateTester<int>> ==
              "TemplateTester<int>");
static_assert(skydown::short_type_name<MyClass> == "MyClass");
}  // namespace simple_type_name_testing
}  // namespace skydown
