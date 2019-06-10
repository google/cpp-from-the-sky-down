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
#include <string_view>
#include <array>

namespace simple_type_name {

namespace detail {
template <typename T>
struct simple_type_name_dummy_type_to_string {};

#if !defined(__clang__) && !defined(__GNUC__) && defined(_MSC_VER)
#define SIMPLE_TYPE_NAME_PRETTY_FUNCTION __FUNCSIG__
#else
#define SIMPLE_TYPE_NAME_PRETTY_FUNCTION __PRETTY_FUNCTION__

#endif

template <typename T>
constexpr auto type_to_string_raw() {
  std::array<char, sizeof(SIMPLE_TYPE_NAME_PRETTY_FUNCTION)> ar{};
  for (std::size_t i = 0; i < ar.size(); ++i) {
    ar[i] = SIMPLE_TYPE_NAME_PRETTY_FUNCTION[i];
  }
  return ar;
}

#undef SIMPLE_TYPE_NAME_PRETTY_FUNCTION

template <typename T>
constexpr decltype(type_to_string_raw<simple_type_name_dummy_type_to_string<T>>())
    raw_type_function_name = type_to_string_raw<simple_type_name_dummy_type_to_string<T>>();

template <typename T>
constexpr std::string_view long_name() {
  std::string_view t("simple_type_name_dummy_type_to_string");
  std::string_view raw_str(raw_type_function_name<T>.data(),
                           raw_type_function_name<T>.size());
  auto pos = raw_str.find(t);
  raw_str.remove_prefix(pos + t.size());
  int last = 0;
  int count = 0;
  for (auto c : raw_str) {
    if (c == '<') ++count;
    if (c == '>') --count;
    if (count == 0) break;
    ++last;
  }
  raw_str.remove_suffix(raw_str.size() - last);
  raw_str.remove_prefix(1);

  std::string_view struct_name("struct ");
  std::string_view class_name("class ");

  if (raw_str.substr(0, struct_name.size()) == struct_name)
    raw_str.remove_prefix(struct_name.size());
   if (raw_str.substr(0, class_name.size()) == class_name)
    raw_str.remove_prefix(class_name.size());
  
  return raw_str;
}

template <typename T>
constexpr std::string_view short_name() {
  auto raw_str = long_name<T>();
  auto pos = raw_str.find_last_of(':');
  if (pos != std::string::npos) {
    raw_str.remove_prefix(pos + 1);
  }
  return raw_str;
}

}  // namespace detail

template <typename T>
inline constexpr std::string_view short_name = detail::short_name<T>();

template <typename T>
inline constexpr std::string_view long_name = detail::long_name<T>();
}  // namespace simple_type_name

