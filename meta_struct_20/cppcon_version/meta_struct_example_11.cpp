#include "meta_struct.h"

#include <iostream>
#include <string>


template <typename MetaStruct>
void print(std::ostream& os, const MetaStruct& ms) {
  ftsd::meta_struct_apply(
      [&](const auto&... m) {
        auto print_item = [&](auto& m) {
          std::cout << m.tag().sv() << ":" << m.value << "\n";
        };
        (print_item(m), ...);
      },
      ms);
};

using NameAndIdArgs = ftsd::meta_struct<     //
    ftsd::member<"name", std::string_view>,  //
    ftsd::member<"id", const int&>           //
    >;

void print_name_id(NameAndIdArgs args) {
  std::cout << "Name is " << get<"name">(args) << " and id is "
            << get<"id">(args) << "\n";
}

enum class encoding : int { fixed = 0, variable = 1 };

int main() {
  using Person = ftsd::meta_struct<                                               //
      ftsd::member<"id", int, ftsd::required, {ftsd::arg<"encoding"> = encoding::variable}>,  //
      ftsd::member<"name", std::string, ftsd::required>,                                //
      ftsd::member<"score", int, [](auto& self) { return ftsd::get<"id">(self) + 1; }>  //
      >;

  constexpr auto attributes = ftsd::get_attributes<"id", Person>();

  if constexpr (ftsd::has<"encoding">(attributes) &&
                ftsd::get<"encoding">(attributes) == encoding::variable) {
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

  Person p{ftsd::arg<"id"> = 2, ftsd::arg<"name"> = "John"};

  using NameAndId = ftsd::meta_struct<         //
      ftsd::member<"name", std::string_view>,  //
      ftsd::member<"id", int>                  //
      >;

  print(p);
  std::cout << "\n";
  NameAndId n = p;
  print(n);
  std::cout << "\n";

  print_name_id(p);
  print_name_id(n);

  static_assert(ftsd::meta_struct_size<NameAndId>() == 2);
  static_assert(ftsd::meta_struct_size(n) == 2);

  constexpr auto fs = ftsd::fixed_string<4>::from_string_view("John");
}
