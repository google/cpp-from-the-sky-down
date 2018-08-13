// Copyright 2018 Google LLC

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     https://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

class string_rep {
 public:
  template <typename Iter1, typename Iter2>
  string_rep(Iter1 b, Iter2 e, std::size_t capacity)
      : ptr_(capacity ? std::make_unique<char[]>(capacity) : nullptr),
        size_(std::distance(b, e)),
        capacity_(capacity) {
    if (capacity_) {
      std::copy(b, e, ptr_.get());
      ptr_.get()[size_] = 0;
    }
  }

  string_rep() = default;
  string_rep(const string_rep& other)
      : string_rep(other.data(), other.data() + other.size(),
                   other.capacity()) {}
  string_rep(string_rep&&) = default;
  string_rep& operator=(string_rep&&) = default;
  string_rep& operator=(const string_rep& other) {
    auto temp = other;
    return *this = std::move(temp);
  }

  std::size_t size() const { return size_; }
  void size(std::size_t size) { size_ = size; }
  std::size_t capacity() const { return capacity_; }
  const char* data() const { return ptr_.get(); }
  char* data() { return ptr_.get(); }

 private:
  std::unique_ptr<char[]> ptr_;
  std::size_t size_ = 0;
  std::size_t capacity_ = 0;
};

class string {
 public:
  string() = default;
  string(const char* s) : rep_(s, s + std::strlen(s), std::strlen(s) + 1) {}

  auto data() const { return rep_.data(); }
  auto data() { return rep_.data(); }
  auto size() const { return rep_.size(); }
  auto capacity() const { return rep_.capacity(); }
  auto begin() const { return data(); }
  auto begin() { return data(); }
  auto end() const { return begin() + size(); }
  auto end() { return begin() + size(); }
  void reserve(size_t new_capacity) {
    if (new_capacity > capacity()) {
      rep_ = string_rep(begin(), end(), new_capacity);
    }
  }

  string& operator+=(const string& other) {
    auto new_size = size() + other.size();
    reserve(new_size + 1);
    std::copy(other.begin(), other.end(), end());
    data()[new_size] = 0;
    rep_.size(new_size);
    return *this;
  }

 private:
  string_rep rep_;
};

inline string operator+(string a, const string& b) {
  a += b;
  return a;
}

inline bool operator==(const string& a, const string& b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

inline std::ostream& operator<<(std::ostream& os, const string& s) {
  os.write(s.data(), s.size());
  return os;
}

template <typename... Sizes>
size_t add(Sizes... sizes) {
  return (0 + ... + sizes);
}

template <typename... String>
string concat(string first, String&&... strings) {
  first.reserve(add(1, strings.size()...));
  return (first += ... += strings);
}

int main() {
  string a = "Hello world";
  string hi = "hi ";
  string there = "there";
  auto b = a + hi + there;
  if (a == "Hello world") {
    std::cout << "Yes\n";
  }

  std::cout << b;
  std::cout << std::boolalpha << (b == concat(a, hi, there));
}
