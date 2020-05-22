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

int main() {
  using namespace skydown::literals;
  sqlite3 *sqldb;
  sqlite3_open(":memory:", &sqldb);

  skydown::prepared_statement<
      "CREATE TABLE customers("
      "id INTEGER NOT NULL PRIMARY KEY, "
      "name TEXT NOT NULL"
      ");"  //
      >{sqldb}
      .execute();

  skydown::prepared_statement<
      "CREATE TABLE orders("
      "id INTEGER NOT NULL PRIMARY KEY,"
      "item TEXT NOT NULL, "
      "customerid INTEGER NOT NULL,"
      "price REAL NOT NULL, "
      "discount_code TEXT "
      ");"  //
      >{sqldb}
      .execute();

  skydown::prepared_statement<
      "INSERT INTO customers(id, name) "
      "VALUES(?id:int, ?name:string);"  //
      >{sqldb}
      .execute("id"_param = 1, "name"_param = "John");

  skydown::prepared_statement<
      "INSERT INTO orders(item , customerid , price, discount_code ) "
      "VALUES (?item:string, ?customerid:int, ?price:double, ?discount_code:string? );"  //
      >
      insert_order{sqldb};

  insert_order.execute("item"_param = "Phone", "price"_param = 1444.44,
                       "customerid"_param = 1);
  insert_order.execute("item"_param = "Laptop", "price"_param = 1300.44,
                       "customerid"_param = 1);
  insert_order.execute("customerid"_param = 1, "price"_param = 2000,
                       "item"_param = "MacBook", "discount_code"_param = "BIGSALE");

  skydown::prepared_statement<
      "SELECT orders.id:int, name:string, item:string, price:double, discount_code:string? "
      "FROM orders JOIN customers ON customers.id = customerid "
      "WHERE price > ?min_price:double;">
      select_orders{sqldb};

  for (;;) {
    std::cout << "Enter min price.\n";
    double min_price = 0;
    std::cin >> min_price;

    for (auto &row :
         select_orders.execute_rows("min_price"_param = min_price)) {
      // Access the fields using by indexing the row with the column (`_col`).
      // We will get a compiler error if we try to access a column that is not
      // part of the select statement.
      std::cout << row["orders.id"_col] << " ";
      std::cout << row["price"_col] << " ";
      std::cout << row["name"_col] << " ";
      std::cout << row["item"_col] << " ";
      std::cout << row["discount_code"_col].value_or("<NO CODE>") << "\n";
    }
  }
}
