use proc_macro::TokenStream;

use syn::parse::{Parse, ParseStream, Result};
use syn::{parse_macro_input, Ident, LitStr, Token};

struct TaggedSql {
    name: String,
    sql: String,
}

impl Parse for TaggedSql {
    fn parse(input: ParseStream) -> Result<Self> {
        let name_ident: Ident = input.parse()?;
        input.parse::<Token![,]>()?;

        let sql_lit: LitStr = input.parse()?;
        Ok(Self {
            name: name_ident.to_string(),
            sql: sql_lit.value(),
        })
    }
}

/// Example of [function-like procedural macro][1].
///
/// [1]: https://doc.rust-lang.org/reference/procedural-macros.html#function-like-procedural-macros
#[proc_macro]
pub fn tagged_sql(input: TokenStream) -> TokenStream {
    //    tagged_sql_impl(input).into()
    let tagged_sql = parse_macro_input!(input as TaggedSql);

    let tokens = tagged_rusqlite_proc_impl::tagged_sql(&tagged_sql.name, &tagged_sql.sql);

    tokens.into()
}
