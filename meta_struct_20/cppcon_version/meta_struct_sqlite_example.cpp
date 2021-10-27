// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>

#include "meta_struct_sqlite.h"
int main() {
  using ftsd::arg;

  sqlite3 *sqldb;
  sqlite3_open(":memory:", &sqldb);

  ftsd::prepared_statement<  //
      R"( CREATE TABLE customers(
      id INTEGER NOT NULL PRIMARY KEY, 
      name TEXT NOT NULL
      );)"                   //
      >{sqldb}
      .execute();

  ftsd::prepared_statement<  //
      R"( CREATE TABLE orders(
      id INTEGER NOT NULL PRIMARY KEY,
      item TEXT NOT NULL, 
      customerid INTEGER NOT NULL,
      price REAL NOT NULL, 
      discount_code TEXT 
      );)"                   //
      >{sqldb}
      .execute();

  ftsd::prepared_statement<
      R"(INSERT INTO customers(name) 
      VALUES(? /*:name:text*/);)"  //
      >{sqldb}
      .execute({arg<"name"> = "John"});

  auto customer_id_or = ftsd::prepared_statement<
                            R"(SELECT id /*:integer*/ from customers 
                            WHERE name = ? /*:name:text*/;)"  //
                            >{sqldb}
                            .execute_single_row({arg<"name"> = "John"});

  if (!customer_id_or) {
    std::cerr << "Unable to find customer name\n";
    return 1;
  }
  auto customer_id = get<"id">(customer_id_or.value());

  ftsd::prepared_statement<
      R"(INSERT INTO orders(item , customerid , price, discount_code ) 
      VALUES (?/*:item:text*/, ?/*:customerid:integer*/, ?/*:price:real*/, 
      ?/*:discount_code:text?*/ );)"  //
      >
      insert_order{sqldb};

  insert_order.execute({arg<"item"> = "Phone", arg<"price"> = 1444.4,
                        arg<"customerid"> = customer_id});
  insert_order.execute({arg<"item"> = "Laptop", arg<"price"> = 1300.4,
                        arg<"customerid"> = customer_id});
  insert_order.execute({arg<"customerid"> = customer_id, arg<"price"> = 2000,
                        arg<"item"> = "MacBook",
                        ftsd::arg<"discount_code"> = "BIGSALE"});

  ftsd::prepared_statement<
      R"(SELECT orders.id /*:integer*/, name/*:text*/, item/*:text*/, 
      price/*:real*/, 
      discount_code/*:text?*/ 
      FROM orders JOIN customers ON customers.id = customerid 
      WHERE price > ?/*:min_price:real*/;)"  //
      >
      select_orders{sqldb};

  for (;;) {
    std::cout << "Enter min price.\n";
    double min_price = 0;
    std::cin >> min_price;

    for (auto &row :
         select_orders.execute_rows({arg<"min_price"> = min_price})) {
      // Access the fields using by indexing the row with the column (`_col`).
      // We will get a compiler error if we try to access a column that is not
      // part of the select statement.
      std::cout << get<"orders.id">(row) << " ";
      std::cout << get<"price">(row) << " ";
      std::cout << get<"name">(row) << " ";
      std::cout << get<"item">(row) << " ";
      std::cout << ftsd::get<"item">(row) << " ";
      std::cout << get<"discount_code">(row).value_or("<NO CODE>") << "\n";
    }
  }
}
