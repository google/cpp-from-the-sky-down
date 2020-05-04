// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"

inline constexpr std::string_view

    // Queries
    create_customers = R"(
CREATE TABLE customers(id INTEGER NOT NULL PRIMARY KEY, name TEXT);
)",
    create_orders = R"(
CREATE TABLE orders(id INTEGER NOT NULL PRIMARY KEY, item TEXT, customerid INTEGER, price REAL);
)",
    insert_customer = R"(
INSERT INTO customers(id, name) VALUES( {{?id:int}}, {{?name:string}});
)",
    insert_order = R"(
INSERT INTO orders(item , customerid , price ) 
VALUES ({{?item:string}},{{?customerid:int}},{{?price:double}});
)",
    select_orders = R"(
SELECT  {{orders.id:int}}, {{name:string}},  {{item:string?}}, {{price:double}}
FROM orders JOIN customers ON customers.id = customerid where price > {{?price:double}}
)",

    // Tags used in the queries (i.e. all the stuff wrapped in '{{ }}').
    customers = "customers", id = "id", name = "name", orders = "orders",
    item = "item", customerid = "customerid", price = "price";

int main() {
  sqlite3 *sqldb;
  sqlite3_open(":memory:", &sqldb);

  using skydown::bind;

  skydown::prepare<create_customers>(sqldb).execute();
  skydown::prepare<create_orders>(sqldb).execute();
  skydown::prepare<insert_customer>(sqldb).execute(bind<id>(1),
                                                   bind<name>("John"));

  auto prepared_insert_order = skydown::prepare<insert_order>(sqldb);

  prepared_insert_order.execute(bind<item>("Phone"), bind<price>(1444.44),
                                bind<customerid>(1));
  prepared_insert_order.execute(bind<item>("Laptop"), bind<price>(1300.44),
                                bind<customerid>(1));
  prepared_insert_order.execute(bind<customerid>(1), bind<price>(2000),
                                bind<item>("MacBook"));

  using skydown::field;

  auto prepared_select_orders = skydown::prepare<select_orders>(sqldb);

  for (;;) {
    std::cout << "Enter min price.\n";
    double min_price = 0;
    std::cin >> min_price;

    for (auto &row :
         prepared_select_orders.execute_rows(bind<price>(min_price))) {
      std::cout << field<orders, id>(row) << " ";
      std::cout << field<price>(row) << " ";
      std::cout << field<name>(row) << " ";
      // Because we had a '?' in the tag for item, the type is std::optional.
      std::cout << field<item>(row).value() << "\n";
    }
  }
}
