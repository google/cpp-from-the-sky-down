// Copyright 2021 Google LLC
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
// limitations under the License.

mod lib;

use rusqlite::{Connection, Result};
use tagged_rusqlite::tagged_sql;

fn main() -> Result<()> {
    let conn = Connection::open_in_memory()?;

    tagged_sql!(
        CreateCustomers,
        r#"CREATE TABLE customers (
                  id              INTEGER PRIMARY KEY,
                  name            TEXT NOT NULL
                  );"#
    );

    CreateCustomers::prepare(&conn).execute()?;

    tagged_sql!(
        CreateOrders,
        r#"CREATE TABLE orders (
                  id              INTEGER PRIMARY KEY,
                  item            TEXT NOT NULL,
                  customerid      INTEGER NOT NULL,
                  price           REAL NOT NULL,
                  discount_code   TEXT
                  );"#
    );

    CreateOrders::prepare(&conn).execute()?;

    tagged_sql!(
        InsertCustomer,
        r#"INSERT INTO  customers (name)
                VALUES (
                    ? /*:name:String*/
                );
                "#
    );

    InsertCustomer::prepare(&conn).execute_bind(&InsertCustomerParams {
        name: "John".to_string(),
    })?;

    tagged_sql!(
        InsertOrder,
        r#"
        INSERT into orders(item, customerid, price, discount_code) 
        VALUES(
                ? /*:item:String*/,
                ? /*:customerid:i64*/,
                ? /*:price:f64*/,
                ? /*:discount_code:Option<String>*/
        )
        "#
    );

    tagged_sql!(
        FindCustomer,
        r#"
        SELECT id /*:i64*/ from customers
        where name = ? /*:name:String*/
        "#
    );

    let customerid: i64 = FindCustomer::prepare(&conn)
        .query_row_bind(&FindCustomerParams {
            name: "John".into(),
        })?
        .id;

    let mut insert_order = InsertOrder::prepare(&conn);
    insert_order.execute_bind(&InsertOrderParams {
        item: "Phone".into(),
        price: 1444.44,
        customerid,
        discount_code: None,
    })?;
    insert_order.execute_bind(&InsertOrderParams {
        item: "Laptop".into(),
        price: 1300.44,
        customerid,
        discount_code: None,
    })?;
    insert_order.execute_bind(&InsertOrderParams {
        item: "Laptop".into(),
        price: 2000.0,
        customerid,
        discount_code: Some("BIGSALE".to_owned()),
    })?;

    tagged_sql!(
        FindOrders,
        r#"
        SELECT
            orders.id as oid /*:i64*/,
            name /*:String*/,
            item /*:String*/,
            price /*:f64*/,
            discount_code /*:Option<String>*/
            FROM orders JOIN customers on customers.id = customerid
            WHERE price > ?/*:min_price:f64*/
        "#
    );

    let mut find_orders = FindOrders::prepare(&conn);
    for orders in find_orders.query_bind(&FindOrdersParams { min_price: 100.0 })? {
        let orders = orders?;
        println!(
            "{} {} {} {} {:?}",
            &orders.oid, &orders.name, &orders.item, &orders.price, &orders.discount_code
        );
    }
    Ok(())
}
