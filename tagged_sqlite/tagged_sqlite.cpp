// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"
#include <exception>

sqlite3 *init_database() {
  sqlite3 *sqldb;
  sqlite3_open(":memory:", &sqldb);

  auto const creation = R"(
CREATE TABLE customers(id PRIMARY_KEY, name TEXT);
CREATE TABLE orders(id PRIMARY_KEY, item TEXT, customerid INTEGER, price REAL);
)";

  auto const insertion = R"(
INSERT INTO customers(id, name) VALUES(1, "John");
INSERT INTO orders(id,  item , customerid , price ) VALUES (1,"Laptop",1,122.22), (2,"Phone",1,444.44);
)";

  int rc;
  rc = sqlite3_exec(sqldb, creation, 0, 0, nullptr);
  skydown::check_sqlite_return(rc);

  rc = sqlite3_exec(sqldb, insertion, 0, 0, nullptr);
  skydown::check_sqlite_return(rc);

  return sqldb;
}

template <typename... Fields>
std::string field_names() {
  auto s = (... + (std::string(simple_type_name::short_name<Fields>) + "."));
  if (s.size() > 0) {
    s.pop_back();
  }
  return s;
}

template <typename... Fields, typename Row>
std::ostream &print_field(std::ostream &os, const Row &row) {
  os << field_names<Fields...>() << ": ";
  auto &val = field<Fields...>(row);
  if (val.has_value()) {
    os << val.value();
  } else {
    os << "NULL";
  }
  os << "\n";
  return os;
}

inline std::string get_name_from_user(){
  return "John";
}


int main() {
    using skydown::define_database;
    using skydown::define_table;
    using skydown::define_column;
    using skydown::table;
    using skydown::column;
    using skydown::field;
    using skydown::query_t;
    using skydown::execute_query;
    using skydown::select;
  using db = define_database<
      define_table<class customers,  //
                   define_column<class id, std::int64_t>,
                   define_column<class name, std::string>>,
      define_table<class orders,  //
                   define_column<class id, std::int64_t>,
                   define_column<class item, std::string>,
                   define_column<class customerid, std::int64_t>,
                   define_column<class price, double>>>;

  query_t query =
      select<db>(column<customers, id>, column<customerid>,
                 column<customers, name>, column<orders, id>,
                 column<orders, item>, column<price>)
          .from(
              table<orders>.join(table<customers>).on(column<customers, id> == column<customerid>))
          .where(column<price> > skydown::parameter<class Parm,double>() && column<customers, name> == get_name_from_user());

  auto sqldb = init_database();
  for (auto &row : execute_query(query, sqldb, skydown::parameter<Parm,double>(20) )) {
    std::cout << field<customers, name>(row).value_or("<NULL>") << "\t"
              << field<orders, item>(row).value_or("<NULL>") << "\t"
              << field<price>(row).value_or(0.0) << "\n";
  }
  std::cout << "\nThe sql statement is:\n" << to_statement(query.t_) << "\n";
  std::cout << "\nThe parameters to the query are:\n";
  std::cout << skydown::get<skydown::expression_parts::arguments>(query.t_);
  std::cout << query.t_;

  {
      static constexpr std::string_view q1= "MyColumn:int?";
      static constexpr std::string_view q2= "New:int";
      auto m1 = skydown::sqlite_experimental::make_member_sv<q1>();
      auto m2 = skydown::sqlite_experimental::make_member_sv<q2>();
      skydown::tagged_tuple t{m1,m2};
      std::cout << t;

      static constexpr std::string_view type_specs= "<:MyColumn:int?> <:New:int>";
      constexpr auto count =
          skydown::sqlite_experimental::get_type_spec_count<type_specs>("<:",">");
      std::cout << "count " << count << "\n";

      auto t1 = skydown::sqlite_experimental::make_members<type_specs>();

      std::cout << t1;
      static constexpr std::string_view sql=
          "SELECT <:id:int>, <:item:string?>  FROM orders ";

      using skydown::sqlite_experimental::fld;
  for (auto &row : skydown::sqlite_experimental::execute_query_string<sql>(sqldb )) {
      class id; class item;
    std::cout << fld<id>(row) << " ";
    std::cout << fld<item>(row).value() << "\n";
  }
  }
  return 0;
}
