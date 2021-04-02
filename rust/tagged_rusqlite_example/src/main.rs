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
