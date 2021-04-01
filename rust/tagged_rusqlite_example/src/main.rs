use tagged_rusqlite_proc::tagged_sql;
use rusqlite::{params, Connection, Result, Statement};

struct StatementHolder<'a>{
    stmt:Statement<'a>,
}

impl<'a> StatementHolder<'a>{
    pub fn new(connection: &'a Connection,s:&str)->Self{
        StatementHolder{
            stmt:connection.prepare(s).unwrap()
        }
    }
}

#[derive(Debug)]
struct Person {
    id: i32,
    name: String,
    data: Option<Vec<u8>>,
}



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

    let mut stmt = StatementHolder::new(&conn,APerson::sql_str());
    let person_iter = stmt.stmt.query_map(params![], APerson::map_fn())?;

    for person in person_iter {
        println!("Found person {:?}", person.unwrap());
    }
    Ok(())
}