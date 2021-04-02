use tagged_rusqlite::tagged_sql;
use rusqlite::{params, Connection, Result};



#[derive(Debug)]
struct Person {
    id: i32,
    name: String,
    data: Option<Vec<u8>>,
}

tagged_sql!(SelectPerson,r#"SELECT
                            id   /*:i64*/,
                            name /*:String*/,
                            data /*:Option<Vec<u8>>*/
                            FROM person
                            "#);


tagged_sql!(InsertPerson, r#"INSERT INTO
                person (name, data)
                VALUES (
                    ?1 /*:name:String*/,
                    ?2 /*:data:Option<Vec<u8>>*/
                )
                "#);

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

    let mut insert = InsertPerson::prepare(&conn);
    insert.execute_with(&InsertPersonParams{name:"Steven".to_string(),data:None})?;
    insert.execute_with(&InsertPersonParams{name:"John".to_string(),data:None})?;
    insert.execute_with(&InsertPersonParams{name:"Bill".to_string(),data:None})?;

    let mut stmt = SelectPerson::prepare(&conn);
    let person_iterator = stmt.query()?;

    for person in  person_iterator{
        println!("Found person {:?}", &person.unwrap());
    }
    Ok(())
}