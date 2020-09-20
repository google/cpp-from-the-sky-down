#include <iostream>
#include <string>

#include "tagged_struct.h"



int main() {


  tagged_struct<member<"hello", auto_, []{return 5;}>, member<"world", std::string>> ts{
      tag<"hello"> = 1};

  using T = decltype(ts);
  T t2{tag<"world"> = std::string("JRB")};

  auto hello = tag<"hello">;
  std::cout << ts->*tag<"hello"> << "\n";
  std::cout << t2->*tag<"hello"> << "\n";
  std::cout << t2->*hello << "\n";
  std::cout << t2->*tag<"world"> << "\n";
}