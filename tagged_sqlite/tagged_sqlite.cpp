// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"

#include <exception>

sqlite3 *init_database() {
  sqlite3 *sqldb;
  sqlite3_open(":memory:", &sqldb);

  class name;
  class id;
  class item;
  class price;
  class customerid;
  using skydown::sqlite_experimental::bind;

  constexpr static std::string_view create_customers = R"(
CREATE TABLE customers(id INTEGER NOT NULL PRIMARY KEY, name TEXT);
)";

  constexpr static std::string_view create_orders = R"(
CREATE TABLE orders(id INTEGER NOT NULL PRIMARY KEY, item TEXT, customerid INTEGER, price REAL);
)";

constexpr static std::string_view insert_customer
  = R"(
INSERT INTO customers(id, name) VALUES( {{?id:int}}, {{?name:string}});
)";


constexpr static std::string_view insert_order = R"(
INSERT INTO orders(item , customerid , price ) 
VALUES ({{?item:string}},{{?customerid:int}},{{?price:double}});
)";

  skydown::sqlite_experimental::prepare<create_customers>(sqldb).execute();
  skydown::sqlite_experimental::prepare<create_orders>(sqldb).execute();
  skydown::sqlite_experimental::prepare<insert_customer>(sqldb).execute(bind<id>(1),bind<name>("John"));

  auto prepared_insert_orders =
      skydown::sqlite_experimental::prepare<insert_order>(sqldb);
  
  prepared_insert_orders.execute(bind<item>("Phone"), bind<price>(1444.44), bind<customerid>(1));
  prepared_insert_orders.execute(bind<item>("Laptop"), bind<price>(1300.44), bind<customerid>(1));
  


  return sqldb;
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
      skydown::sqlite_experimental::prepare<insert_sql>(sqldb);
  insert_statement.execute(bind<customerid>(1), bind<price>(2000),
                                    bind<item>("MacBook"));
  auto select_statement =
      skydown::sqlite_experimental::prepare<select_sql>(sqldb);

  for (;;) {
    std::cout << "Enter min price.\n";
    double min_price = 0;
    std::cin >> min_price;

    for (auto &row : select_statement.execute_rows(bind<price>(min_price))) {
      std::cout << field<orders, id>(row) << " ";
      std::cout << field<price>(row) << " ";
      std::cout << field<name>(row) << " ";
      std::cout << field<item>(row).value() << "\n";
    }
  }

}
