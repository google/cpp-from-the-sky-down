// tagged_sqlite.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include "..//simple_type_name/simple_type_name.h"
#include "..//tagged_tuple/tagged_tuple.h"

template <typename Tag, typename... Tables>
struct define_database:Tables... {};

template <typename Tag, typename... Columns>
struct define_table:Columns... {};

template <typename Tag, typename Type>
struct define_column {};

namespace detail{
	template<typename Tag, typename... Tables>
std::true_type test_has_table(const define_database<Tag, Tables...>&);
std::false_type test_has_table(...);

template<typename Database, typename Tag>
inline constexpr bool has_table = decltype(test_has_table(Database{}))::value;

template<typename Tag, typename... Columns>
std::true_type test_has_column(const define_table<Tag, Columns...>&);
std::false_type test_has_column(...);

template<typename Table, typename Tag>
inline constexpr bool has_column = decltype(test_has_column(Table{}))::value;





}

using db = define_database<
    class mydb,
    define_table<class customers, define_column<class id, std::int64_t>,
                 define_column<class name, std::string>>,
    define_table<class orders, define_column<class id, std::int64_t>,
                 define_column<class item, std::string>,
                 define_column<class customerid, std::int64_t>>>;

template <typename Alias, typename ColumnName, typename TableName>
struct column_alias_ref {};

template <typename ColumnName, typename TableName = void>
struct column_ref {
  template <typename Alias>
  static constexpr column_alias_ref<Alias, ColumnName, TableName> as = {};
};

template <typename N1, typename N2>
struct column_ref_definer {
  using type = column_ref<N2, N1>;
};

template <typename N1>
struct column_ref_definer<N1, void> {
  using type = column_ref<N1, void>;
};

template <typename... ColumnRefs>
struct column_ref_holder {};

template <typename ColumnRef, typename NewName>
struct as_ref {};

template <typename N1, typename N2 = void>
inline constexpr auto column = typename column_ref_definer<N1, N2>::type{};

template <typename Table>
struct table_ref {};

template <typename Table>
inline constexpr auto table = table_ref<Table>{};

template <typename... Tables>
struct from_type {};

template <typename... ColumnRefs>
struct select_type {};

template <typename T1, typename T2>
struct catter_imp;

template <typename... M1, typename... M2>
struct catter_imp<tagged_tuple::tagged_tuple<M1...>,
                  tagged_tuple::tagged_tuple<M2...>> {
  using type = tagged_tuple::tagged_tuple<M1..., M2...>;
};

template <typename T1, typename T2>
using cat_t = typename catter_imp<T1, T2>::type;

template <typename Database, typename TTuple = tagged_tuple::tagged_tuple<>>
struct query_builder {
  template <typename... Tables>
  static query_builder<
      Database, cat_t<TTuple, tagged_tuple::tagged_tuple<tagged_tuple::member<
                                  class from_tag, from_type<Tables...>>>>>
  from(table_ref<Tables>...) {
    return {};
  }

  template <typename... Columns>
  static query_builder<
      Database, cat_t<TTuple, tagged_tuple::tagged_tuple<tagged_tuple::member<
                                  class select_tag, select_type<Columns...>>>>>
  select(Columns...) {
    return {};
  }
};

template <typename Column, typename Table>
std::string to_column_string(column_ref<Column, Table>) {
  if constexpr (std::is_same_v<Table, void>) {
    return std::string(simple_type_name::short_name<Column>);
  } else {
    return std::string(simple_type_name::short_name<Table>) + "." +
           std::string(simple_type_name::short_name<Column>);
  }
}

template <typename Alias, typename Column, typename Table>
std::string to_column_string(column_alias_ref<Alias, Column, Table>) {
  return to_column_string(column_ref<Column, Table>{}) + " AS " +
         std::string(simple_type_name::short_name<Alias>);
}

#include <vector>

inline std::string join_vector(const std::vector<std::string>& v) {
  std::string ret;
  for (auto& s : v) {
    ret += s + ",";
  }
  if (ret.back() == ',') ret.pop_back();

  return ret;
}

template <typename... Columns>
std::string to_statement(select_type<Columns...>) {
  std::vector<std::string> v{to_column_string(Columns{})...};

  return std::string("SELECT ") + join_vector(v);
}

template <typename... Tables>
std::string to_statement(from_type<Tables...>) {
  std::vector<std::string> v{
      std::string(simple_type_name::short_name<Tables>)...};
  return std::string("\nFROM ") + join_vector(v);
}

template <typename Database, typename TTuple>
std::string to_statement(query_builder<Database, TTuple> t) {
  std::string ret;
  if constexpr (tagged_tuple::has_tag<select_tag, TTuple>) {
    ret += to_statement(get<select_tag>(TTuple{}));
  }

  if constexpr (tagged_tuple::has_tag<from_tag, TTuple>) {
    ret += to_statement(get<from_tag>(TTuple{}));
  }
  return ret;
}

#include <iostream>

// TODO: Reference additional headers your program requires here.
