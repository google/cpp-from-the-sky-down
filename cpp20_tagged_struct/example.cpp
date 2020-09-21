#include <iostream>
#include <string>

#include "tagged_struct.h"

using test_arguments =
    tagged_struct<member<"a", int, [] {}>,  // Required argument.
                  member<"b", auto_, [](auto& t) { return get<"a">(t) + 2; }>,
                  member<"c", auto_, [](auto& t) { return get<"b">(t) + 2; }>>;

void test(test_arguments args) {
  std::cout << get<"a">(args) << "\n";
  std::cout << get<"b">(args) << "\n";
  std::cout << get<"c">(args) << "\n";
}

int main() {
  tagged_struct<member<"hello", auto_, [] { return 5; }>,
                member<"world", std::string, [] {}>,
                member<"test", auto_,
                       [](auto& t) {
                         return 2 * get<"hello">(t) + get<"world">(t).size();
                       }>,
                member<"last", int, [] { return 0; }>>
      ts{arg<"world"> = "Universe", arg<"hello"> = 1};

  using T = decltype(ts);
  T t2{arg<"world"> = "JRB"};

  std::cout << get<"hello">(ts) << "\n";
  std::cout << get<"world">(ts) << "\n";
  std::cout << get<"test">(ts) << "\n";
  std::cout << get<"hello">(t2) << "\n";
  std::cout << get<"world">(t2) << "\n";
  std::cout << get<"test">(t2) << "\n";

  test({arg<"c"> = 1, arg<"a"> = 5});
  test({arg<"a"> = 1});
}