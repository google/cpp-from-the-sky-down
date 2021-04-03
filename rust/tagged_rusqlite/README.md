*This library is in a very preliminary state. Mainly posting to see if other people think this might be interesting*

# TLDR:

Annotate your sql statements with special comments, and this library will generate structs that make working with your queries easier.

# Why another SQL library

I was looking for a library with the following criteria:

* Use SQL instead of a DSL for queries
* Don't read environmental variables or connect to databases
* Minimum boilerplate to get started. No need to define a file with the entire schema
* Make query results and parameters strongly typed
* Focus on prepared statements.
* Lightweight

Actually to be honest, I have been playing around with how to do this in C++20 using bleeding edge features like strings as template parameters ( [http://jrb-programming.blogspot.com/2020/05/c20-sql.html](http://jrb-programming.blogspot.com/2020/05/c20-sql.html) ), decided to try to do this in Rust.

This is my very preliminary result.

# Powered by Rusqlite (and SQLite)
Rusqlite does most of the heavy lifting, and this is a rather lightweight veneer on top of it. I started with SQLite because it is super simple and self-contained. If there is interest, it can be extended to other databases.

The example below is also largely taken from the Rusqlite example.

# Main idea

The main idea for me was the I very rarely write SQL in my code de novo or from first principles. Rather, I will play around with some sort of interactive SQL environment. Thus, I want something where it is easy to move the SQL back and forth with a minimum of bother.

# What you do

You call the proc macro `tagged_sql` supplying it with the name of your query, and the sql string, annotated with sql comments (which are old c-style `/* */` comments). You annotate the columns you are selecting with `/*:Type*/` and any parameters with `/*:name:Type*/`.

The library then generates several helper structs. Let's use an example (taken largely from the Rusqlite example with some modifications):

        tagged_sql!(
            SearchByName,
            r#"SELECT
                                id   /*:i64*/,
                                name /*:String*/,
                                data /*:Option<Vec<u8>>*/
                                FROM person
                                WHERE name = ? /*:name:String*/;
                                "#
        );

`SearchByName` is the name of the query; `id,name,data` are the columns we are selecting. We annotate each of those columns with a special comment specifying the type those should be. There is one parameter, which we annotate with a comment saying it should be named `name` and have type `String`.

Notice, because of how the annotations are all comments, this is a valid SQL string, so it is easy to copy and paste it into an interactive SQL tool.

`tagged_sql` will generate 3 classes:

* `SearchByName` which has a public static function `prepare` which takes a connection and returns a prepared statement specialized for this query.

* `SearchByNameRow` which has members corresponding to the columns based on the annotated type.

* `SearchByNameParams` which has members corresponding to the parameters based on the name and type annotations.

The returned statement object has the following methods:

* execute
* execute_bind
* query
* query_bind
* query_row
* query_row_bind

The methods with `bind` all take  `Params`. Otherwise, these are very similar to the Rusqlite methods of Statement.

The full code is at:
https://github.com/google/cpp-from-the-sky-down/blob/master/rust/

Below is the example from:
https://github.com/google/cpp-from-the-sky-down/blob/master/rust/tagged_rusqlite_example/src/main.rs


    use rusqlite::{Connection, Result};
    use tagged_rusqlite::tagged_sql;
    
    fn main() -> Result<()> {
        let conn = Connection::open_in_memory()?;
    
        tagged_sql!(
            CreateTable,
            "CREATE TABLE person (
                      id              INTEGER PRIMARY KEY,
                      name            TEXT NOT NULL,
                      data            BLOB
                      );"
        );
    
        CreateTable::prepare(&conn).execute()?;
    
        tagged_sql!(
            InsertPerson,
            r#"INSERT INTO
                    person (name, data)
                    VALUES (
                        ?1 /*:name:String*/,
                        ?2 /*:data:Option<Vec<u8>>*/
                    );
                    "#
        );
    
        let mut insert = InsertPerson::prepare(&conn);
        insert.execute_bind(&InsertPersonParams {
            name: "Steven".to_string(),
            data: Some(vec![1u8, 2u8]),
        })?;
        insert.execute_bind(&InsertPersonParams {
            name: "John".to_string(),
            data: None,
        })?;
        insert.execute_bind(&InsertPersonParams {
            name: "Bill".to_string(),
            data: None,
        })?;
    
        tagged_sql!(
            SelectPerson,
            r#"SELECT
                                id   /*:i64*/,
                                name /*:String*/,
                                data /*:Option<Vec<u8>>*/
                                FROM person;
                                "#
        );
    
        let mut stmt = SelectPerson::prepare(&conn);
        let person_iterator = stmt.query()?;
    
        for person in person_iterator {
            println!("Found person {:?}", &person.unwrap());
        }
    
        tagged_sql!(
            SearchByName,
            r#"SELECT
                                id   /*:i64*/,
                                name /*:String*/,
                                data /*:Option<Vec<u8>>*/
                                FROM person
                                WHERE name = ? /*:name:String*/;
                                "#
        );
        let search_params = SearchByNameParams {
            name: "John".into(),
        };
        for person in SearchByName::prepare(&conn).query_bind(&search_params)? {
            println!(
                "Found person with name {}  {:?}",
                &search_params.name,
                &person.unwrap()
            );
        }
    
        let mut stmt = SelectPerson::prepare(&conn);
        println!("Found single person {:?} ", stmt.query_row()?);
    
        let mut stmt = SearchByName::prepare(&conn);
        let search_params = SearchByNameParams {
            name: "Bill".into(),
        };
        println!(
            "Found single person with name {}, {:?} ",
            &search_params.name,
            stmt.query_row_bind(&search_params)?
        );
    
        Ok(())
    }
    
    /* Output
    Found person SelectPersonRow { id: 1, name: "Steven", data: Some([1, 2]) }
    Found person SelectPersonRow { id: 2, name: "John", data: None }
    Found person SelectPersonRow { id: 3, name: "Bill", data: None }
    Found person with name John  SearchByNameRow { id: 2, name: "John", data: None }
    Found single person SelectPersonRow { id: 1, name: "Steven", data: Some([1, 2]) }
    Found single person with name Bill, SearchByNameRow { id: 3, name: "Bill", data: None }
     */