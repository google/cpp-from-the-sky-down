#include <iostream>
#include <string>

#include "tagged_struct.h"



int main() {
  using namespace literals;


  tagged_struct<member<"hello", auto_, []{return 5;}>, member<"world", std::string>> ts{
      "hello"_tag = 1};

  using T = decltype(ts);
  T t2{"world"_tag = std::string("JRB")};

  std::cout << ts->*"hello"_tag << "\n";
  std::cout << t2->*"hello"_tag << "\n";
  std::cout << t2->*"world"_tag << "\n";
}