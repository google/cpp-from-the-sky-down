#include <iostream>
#include <string>

#include "tagged_tuple.h"

using ftsd::get;
using ftsd::tagged_tuple;
using namespace ftsd::literals;
using ftsd::auto_;
using ftsd::member;
using ftsd::tag;

using test_arguments =
    tagged_tuple<member<"a", int, [] {}>,  // Required argument.
                 member<"b", auto_, [](auto& t) { return get<"a">(t) + 2; }>,
                 member<"c", auto_, [](auto& t) { return get<"b">(t) + 2; }>,
                 member<"d", auto_, []() { return 5; }>>;

void test(test_arguments args) {
  std::cout << get<"a">(args) << "\n";
  std::cout << get<"b">(args) << "\n";
  std::cout << get<"c">(args) << "\n";
  std::cout << get<"d">(args) << "\n";
}

template <typename T>
void print(const T& t) {
  auto f = [](auto&&... m) {
    auto print = [](auto& m) { std::cout << m.key() << ": " << m.value() << "\n"; };
    (print(m), ...);
  };
  std::cout << "{\n";
  t.apply(f);
  std::cout << "}\n";
}

int main() {
  tagged_tuple<member<"hello", auto_, [] { return 5; }>,
               member<"world", std::string,
                      [](auto& self) { return get<"hello">(self); }>,
               member<"test", auto_,
                      [](auto& t) {
                        return 2 * get<"hello">(t) + get<"world">(t).size();
                      }>,
               member<"last", int>>
      ts{tag<"world"> = "Universe", tag<"hello"> = 1};

  print(ts);

  using T = decltype(ts);
  T t2{tag<"world"> = "JRB"};

  std::cout << get<"hello">(ts) << "\n";
  std::cout << get<"world">(ts) << "\n";
  std::cout << get<"test">(ts) << "\n";
  std::cout << get<"hello">(t2) << "\n";
  std::cout << get<"world">(t2) << "\n";
  std::cout << get<"test">(t2) << "\n";

  test({tag<"c"> = 1, tag<"a"> = 5});
  test({tag<"a"> = 1});
  test({"a"_tag = 1});

  tagged_tuple ctad{"a"_tag = 15, "b"_tag = std::string("Hello ctad")};

  std::cout << ts["world"_tag] << "\n";
  std::cout << ts[tag<"world">] << "\n";
  std::cout << ctad["a"_tag] << "\n";
}