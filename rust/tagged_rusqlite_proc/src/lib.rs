use proc_macro::TokenStream;

use syn::{parse_macro_input, LitStr};
use quote::quote;

/// Example of [function-like procedural macro][1].
///
/// [1]: https://doc.rust-lang.org/reference/procedural-macros.html#function-like-procedural-macros
#[proc_macro]
pub fn tagged_sql(input: TokenStream) -> TokenStream {
//    tagged_sql_impl(input).into()
    let input = parse_macro_input!(input as LitStr);
    let _input_str = input.value();

    let tokens = quote! {

        struct Hello{};
        impl Hello{
            pub fn get_str()->&'static str{
                #input
            }
        }
    };

    tokens.into()

}