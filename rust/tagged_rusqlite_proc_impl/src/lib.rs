
use quote::quote;
pub fn tagged_sql(struct_name_str:&str,sql:&str) ->proc_macro2::TokenStream{
    let v:Vec<_> = sql.split_ascii_whitespace().collect();
    let struct_name = quote::format_ident!("{}",&struct_name_str);
    let struct_name = syn::parse_str::<syn::Type>(&struct_name_str).unwrap();
    let mut select_members = Vec::new();
    for (index,&part) in v.iter().enumerate(){
        if part.starts_with("/*:") && part.ends_with("*/"){
            let middle = part.strip_prefix("/*:").unwrap().strip_suffix("*/").unwrap().trim();
            if !middle.starts_with("?") {
                let name = quote::format_ident!("{}",v[index - 1]);
                let type_parsed = syn::parse_str::<syn::Type>(middle).unwrap();
                select_members.push(
                    quote! {#name:#type_parsed}
                );
            }
        }
    }

    let quoted_sql = format!("\"{}\"",sql);

    quote!{
        struct #struct_name{
            #(#select_members),*

        }
        impl #struct_name{
            pub fn sql_str() ->&'static str{
                #quoted_sql
            }
        }
    }

}