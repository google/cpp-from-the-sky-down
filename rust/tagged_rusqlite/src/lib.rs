pub use tagged_rusqlite_proc::tagged_sql;
pub trait TaggedSqlMembers {
    fn sql_str() -> &'static str;

    fn from_row(row: &rusqlite::Row<'_>) -> rusqlite::Result<Self>
        where
            Self: Sized;
}

pub trait TaggedSqlParams {
    fn to_sql_slice<'a>(self: &'a Self) -> &[&'a dyn rusqlite::ToSql];
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
