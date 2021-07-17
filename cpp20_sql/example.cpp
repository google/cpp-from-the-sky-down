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

#include "tagged_sqlite.h"

#include <iostream>
int main() {
    using ftsd::bind;
    using ftsd::field;

  sqlite3 *sqldb;
  sqlite3_open(":memory:", &sqldb);

  ftsd::prepared_statement<
      "CREATE TABLE customers("
      "id INTEGER NOT NULL PRIMARY KEY, "
      "name TEXT NOT NULL"
      ");"  //
      >{sqldb}
      .execute();

  ftsd::prepared_statement<
      "CREATE TABLE orders("
      "id INTEGER NOT NULL PRIMARY KEY,"
      "item TEXT NOT NULL, "
      "customerid INTEGER NOT NULL,"
      "price REAL NOT NULL, "
      "discount_code TEXT "
      ");"  //
      >{sqldb}
      .execute();

  ftsd::prepared_statement<
      "INSERT INTO customers(name) "
      "VALUES(? /*:name:text*/);"  //
      >{sqldb}
      .execute({bind<"name">("John")});

 auto customer_id_or = ftsd::prepared_statement<
      "select id/*:integer*/ from customers "
      "where name = ? /*:name:text*/;"  //
      >
      {sqldb}.execute_single_row({bind<"name">("John")});

  if(!customer_id_or){
    std::cerr << "Unable to find customer name\n";
    return 1;
  }
  auto customer_id = field<"id">(customer_id_or.value());

  ftsd::prepared_statement<
      "INSERT INTO orders(item , customerid , price, discount_code ) "
      "VALUES (?/*:item:text*/, ?/*:customerid:integer*/, ?/*:price:real*/, "
      "?/*:discount_code:text?*/ );"  //
      >
      insert_order{sqldb};

  insert_order.execute({bind<"item">("Phone"), bind<"price">(1444.44),
                       bind<"customerid">(customer_id)});
  insert_order.execute({bind<"item">("Laptop"), bind<"price">(1300.44),
                       bind<"customerid">(customer_id)});
  insert_order.execute({bind<"customerid">(customer_id), bind<"price">(2000),
                       bind<"item">("MacBook"),
                       ftsd::bind<"discount_code">("BIGSALE")});

  ftsd::prepared_statement<
      "SELECT orders.id /*:integer*/, name/*:text*/, item/*:text*/, price/*:real*/, "
      "discount_code/*:text?*/ "
      "FROM orders JOIN customers ON customers.id = customerid "
      "WHERE price > ?/*:min_price:real*/;">
      select_orders{sqldb};

  for (;;) {
    std::cout << "Enter min price.\n";
    double min_price = 0;
    std::cin >> min_price;

    for (auto &row :
         select_orders.execute_rows({bind<"min_price">(min_price)})) {
      // Access the fields using by indexing the row with the column (`_col`).
      // We will get a compiler error if we try to access a column that is not
      // part of the select statement.
      std::cout << field<"orders.id">(row) << " ";
      std::cout << field<"price">(row) << " ";
      std::cout << field<"name">(row) << " ";
      std::cout << field<"item">(row) << " ";
      std::cout << ftsd::field<"item">(row) << " ";
      std::cout << field<"discount_code">(row).value_or("<NO CODE>") << "\n";
    }
  }
}
