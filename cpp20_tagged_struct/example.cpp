#include <iostream>
#include <string>

#include "tagged_struct.h"
int main() {
  using namespace literals;

  auto F = std::string;

  tagged_struct<member<"hello", int, []{return 5;}>, member<"world", std::string,[]{}>> ts{
      "hello"_tag = 1, "world"_tag = std::string("JRB")};

  using T = decltype(ts);
  T t2{"world"_tag = std::string("JRB")};

  std::cout << ts->*"hello"_tag << "\n";
  std::cout << t2->*"hello"_tag << "\n";
  std::cout << t2->*"world"_tag << "\n";
}