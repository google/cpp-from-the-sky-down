#[cfg(test)]
mod tests {
    use rusqlite::{Connection, Result};
    use tagged_rusqlite::tagged_sql;

    #[test]
    fn smoke_test() -> Result<()> {
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
                )
                RETURNING id /*:i64*/;
                "#
    );

        let mut insert = InsertPerson::prepare(&conn);
        let id = insert.query_row_bind(&InsertPersonParams {
            name: "Steven".to_string(),
            data: Some(vec![1u8, 2u8]),
        })?;
        assert_eq!(id.id,1);
        let id = insert.query_row_bind(&InsertPersonParams {
            name: "John".to_string(),
            data: None,
        })?;
        assert_eq!(id.id,2);
        let id = insert.query_row_bind(&InsertPersonParams {
            name: "Bill".to_string(),
            data: None,
        })?;
        assert_eq!(id.id,3);

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
        let people: Result<Vec<_>, _> = stmt.query()?.collect();
        assert_eq!(people?, vec![SelectPersonRow { id: 1, name: "Steven".into(), data: Some(vec![1u8, 2u8]) },
                                 SelectPersonRow { id: 2, name: "John".into(), data: None },
                                 SelectPersonRow { id: 3, name: "Bill".into(), data: None }, ]);

        tagged_sql!(
        SearchByName,
        r#"SELECT
                            id   /*:i64*/,
                            name /*:String*/,
                            data /*:Option<Vec<u8>>*/
                            FROM person
                            WHERE name = ? /*:name2:String*/;
                            "#
    );
        let search_params = SearchByNameParams {
            name2: "John".into(),
        };
        let people: Result<Vec<_>, _> = SearchByName::prepare(&conn).query_bind(&search_params)?.collect();
        assert_eq!(people?, vec![
            SearchByNameRow { id: 2, name: "John".into(), data: None },
        ]);


        let mut stmt = SelectPerson::prepare(&conn);
        assert_eq!(stmt.query_row()?, SelectPersonRow { id: 1, name: "Steven".into(), data: Some(vec![1u8, 2u8]) });

        let mut stmt = SearchByName::prepare(&conn);
        let search_params = SearchByNameParams {
            name2: "Bill".into(),
        };
        assert_eq!(stmt.query_row_bind(&search_params)?, SearchByNameRow { id: 3, name: "Bill".into(), data: None });
        Ok(())
    }
}