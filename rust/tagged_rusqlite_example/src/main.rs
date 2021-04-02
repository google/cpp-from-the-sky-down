use tagged_rusqlite::tagged_sql;
use rusqlite::{params, Connection, Result};



#[derive(Debug)]
struct Person {
    id: i32,
    name: String,
    data: Option<Vec<u8>>,
}

tagged_sql!(APerson,"SELECT id/*:i64*/, name/*:String*/, data/*:Option<Vec<u8>>*/ FROM person");


tagged_sql!(Insert,"INSERT INTO person (name, data) VALUES (?1/*:name:String*/, ?2/*:data:Option<Vec<u8>>*/)");

fn main() -> Result<()> {
    let conn = Connection::open_in_memory()?;

    conn.execute(
        "CREATE TABLE person (
                  id              INTEGER PRIMARY KEY,
                  name            TEXT NOT NULL,
                  data            BLOB
                  )",
        params![],
    )?;

    let mut insert = Insert::prepare(&conn);
    insert.execute_with(&InsertParams{name:"Steven".to_string(),data:None})?;

    let mut stmt = APerson::prepare(&conn);
    let person_iterator = stmt.query()?;

    for person in  person_iterator{
        println!("Found person {:?}", &person.unwrap());
    }
    Ok(())
}