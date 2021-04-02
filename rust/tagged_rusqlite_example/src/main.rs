use tagged_rusqlite::tagged_sql;
use tagged_rusqlite::TaggedQuery;
use tagged_rusqlite::TaggedRow;
use rusqlite::{params, Connection, Result, Statement};



#[derive(Debug)]
struct Person {
    id: i32,
    name: String,
    data: Option<Vec<u8>>,
}

# [derive (Debug)] struct MyStructRow { name : String } impl tagged_rusqlite :: TaggedRow for MyStructRow { fn sql_str () -> & 'static str { "Select name /*:String*/ ? /*:user:i32*/" } fn from_row (row : &
rusqlite :: Row < '_ >) -> rusqlite :: Result < Self > where Self : Sized { Ok (Self { name : row . get (0usize) ? }) } } struct MyStructParams { user : i32 } struct MyStruct { } impl tagged_rusqlite ::
TaggedQuery for MyStruct { type Row = MyStructRow ; type Params = MyStructParams ; fn sql_str () -> & 'static str { "Select name /*:String*/ ? /*:user:i32*/" } } impl < 'a > MyStruct { pub fn prepare
(connection : & 'a rusqlite :: Connection) -> tagged_rusqlite :: StatementHolder < 'a , MyStruct > { tagged_rusqlite :: StatementHolder :: new (connection) } }

tagged_sql!(APerson,"SELECT id/*:i64*/, name/*:String*/, data/*:Option<Vec<u8>>*/ FROM person");

fn main() -> Result<()> {
    tagged_sql!(Hello,"Select name /*:String*/");
    println!("{}",Hello::sql_str());
    let conn = Connection::open_in_memory()?;

    conn.execute(
        "CREATE TABLE person (
                  id              INTEGER PRIMARY KEY,
                  name            TEXT NOT NULL,
                  data            BLOB
                  )",
        params![],
    )?;
    let me = Person {
        id: 0,
        name: "Steven".to_string(),
        data: None,
    };
    conn.execute(
        "INSERT INTO person (name, data) VALUES (?1, ?2)",
        params![me.name, me.data],
    )?;

    let mut stmt = APerson::prepare(&conn);
    let person_iter = stmt.stmt.query_map(params![], |r|APersonRow::from_row(r))?;

    for person in person_iter {
        println!("Found person {:?}", person.unwrap());
    }
    Ok(())
}