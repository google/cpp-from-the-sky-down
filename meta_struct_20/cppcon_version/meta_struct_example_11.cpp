#include "meta_struct.h"

#include <iostream>
#include <string>

using namespace ftsd;

template <typename MetaStruct>
void print(std::ostream& os, const MetaStruct& ms) {
  meta_struct_apply(
      [&](const auto&... m) {
        auto print_item = [&](auto& m) {
          std::cout << m.tag().sv() << ":" << m.value << "\n";
        };
        (print_item(m), ...);
      },
      ms);
};

using NameAndIdArgs = meta_struct<     //
    member<"name", std::string_view>,  //
    member<"id", const int&>           //
    >;

void print_name_id(NameAndIdArgs args) {
  std::cout << "Name is " << get<"name">(args) << " and id is "
            << get<"id">(args) << "\n";
}

enum class encoding : int { fixed = 0, variable = 1 };

int main() {
  using Person = meta_struct<                                               //
      member<"id", int, required, {arg<"encoding"> = encoding::variable}>,  //
      member<"name", std::string, required>,                                //
      member<"score", int, [](auto& self) { return get<"id">(self) + 1; }>  //
      >;

  constexpr auto attributes = get_attributes<"id", Person>();

  if constexpr (has<"encoding">(attributes) &&
                get<"encoding">(attributes) == encoding::variable) {
    std::cout << "Encoding was variable";
  } else {
    std::cout << "Encoding was fixed";
  }

  auto print = [](auto& t) {
    meta_struct_apply(
        [&](const auto&... m) {
          ((std::cout << m.tag().sv() << ":" << m.value << "\n"), ...);
        },
        t);
  };

  Person p{arg<"id"> = 2, arg<"name"> = "John"};

  using NameAndId = meta_struct<         //
      member<"name", std::string_view>,  //
      member<"id", int>                  //
      >;

  print(p);
  std::cout << "\n";
  NameAndId n = p;
  print(n);
  std::cout << "\n";

  print_name_id(p);
  print_name_id(n);
}
