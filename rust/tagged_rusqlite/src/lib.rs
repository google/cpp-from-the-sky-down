pub use tagged_rusqlite_proc::tagged_sql;
use std::marker::PhantomData;

pub trait NoParams{}

pub trait HasParams{}

pub trait TaggedQuery {
    type Row;
    type Params;
    fn sql_str() -> &'static str;
}

pub trait TaggedRow {
    fn sql_str() -> &'static str;

    fn from_row(row: &rusqlite::Row<'_>) -> rusqlite::Result<Self>
        where
            Self: Sized;
}

pub trait TaggedSqlParams {
    fn to_sql_slice<'a>(self: &'a Self) -> &[&'a dyn rusqlite::ToSql];
}

pub struct StatementHolder<'a,T:TaggedQuery> {
    pub stmt: rusqlite::Statement<'a>,
    fake: PhantomData<T>,
}

impl<'a, T:TaggedQuery> StatementHolder<'a,T>{
    pub fn new(connection:&'a rusqlite::Connection)->Self{
        Self{stmt: connection.prepare(T::sql_str()).unwrap(),fake:Default::default()}
    }
}


#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
