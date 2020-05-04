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

inline std::string get_name_from_user() { return "John"; }

int main() {
  sqlite3 *sqldb = init_database();
  static constexpr std::string_view select_sql =
      R"(
SELECT  {{orders.id:int}}, {{name:string}},  {{item:string?}}, {{price:double}}
FROM orders JOIN customers ON customers.id = customerid where price > {{?price:double}}

)";

  static constexpr std::string_view insert_sql = R"(
INSERT INTO orders( item , customerid , price ) 
VALUES ({{?item:string}},{{?customerid:int}} ,{{?price:double}});
)";
  class customers;
  class orders;
  class id;
  class item;
  class name;
  class customerid;
  class price;
  using skydown::sqlite_experimental::bind;
  using skydown::sqlite_experimental::field;

  auto insert_statement =
      skydown::sqlite_experimental::prepare_query_string<insert_sql>(sqldb);
  auto r = insert_statement.execute(bind<customerid>(1), bind<price>(2000),
                                    bind<item>("MacBook"));
  assert(r);

  auto select_statement =
      skydown::sqlite_experimental::prepare_query_string<select_sql>(sqldb);

  for (;;) {
    std::cout << "Enter min price.\n";
    double min_price = 0;
    std::cin >> min_price;

    auto rows = select_statement.execute_rows(bind<price>(min_price));
    assert(!rows.has_error());

    for (auto &row : rows) {
      std::cout << field<orders, id>(row) << " ";
      std::cout << field<price>(row) << " ";
      std::cout << field<name>(row) << " ";
      std::cout << field<item>(row).value() << "\n";
    }
  }

}
