use std::marker::PhantomData;
pub use tagged_rusqlite_proc::tagged_sql;

pub trait NoParams {}

pub trait HasParams {}

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

pub trait TaggedParams {
    fn bind_all(&self, s:&mut rusqlite::Statement) ->rusqlite::Result<()>;
}

pub struct StatementHolder<'a, T: TaggedQuery> {
    stmt: rusqlite::Statement<'a>,
    fake: PhantomData<T>,
}

impl<'a, T: TaggedQuery> StatementHolder<'a, T> {
    pub fn new(connection: &'a rusqlite::Connection) -> Self {
        Self {
            stmt: connection.prepare(T::sql_str()).unwrap(),
            fake: Default::default(),
        }
    }
}

impl<'a, T: TaggedQuery> StatementHolder<'a, T>
where
    T: TaggedQuery + NoParams,
    T::Row: TaggedRow,
{
    pub fn query(
        &'_ mut self,
    ) -> rusqlite::Result<rusqlite::MappedRows<'_, fn(&rusqlite::Row) -> rusqlite::Result<T::Row>>>
    {
        self.stmt.query_map(rusqlite::params![], T::Row::from_row)
    }

    pub fn execute(
        &'_ mut self,
    ) -> rusqlite::Result<usize>
    {
        self.stmt.execute(rusqlite::params![])
    }
}

impl<'a, T: TaggedQuery> StatementHolder<'a, T>
    where
        T: TaggedQuery,
        T::Row: TaggedRow,
        T::Params: TaggedParams,

{
    pub fn query_bind(
        &'_ mut self,params:&T::Params
    ) -> rusqlite::Result<rusqlite::MappedRows<'_, fn(&rusqlite::Row) -> rusqlite::Result<T::Row>>>
    {

        params.bind_all(&mut self.stmt)?;
        Ok(self.stmt.raw_query().mapped(T::Row::from_row))
    }

    pub fn execute_bind(
        &'_ mut self,params:&T::Params
    ) -> rusqlite::Result<usize>
    {
        params.bind_all(&mut self.stmt)?;
        self.stmt.raw_execute()
    }
}


#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
