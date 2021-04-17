// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// limitations under the License.

use proc_macro2::TokenStream;
use quote::quote;
use regex::Regex;

#[derive(Debug)]
struct SqlMember {
    name: String,
    type_name: String,
}

impl SqlMember {
    fn to_decl(&self) -> TokenStream {
        let name = syn::parse_str::<syn::Ident>(&self.name).unwrap();
        let type_name = syn::parse_str::<syn::Type>(&self.type_name).unwrap();
        quote! {#name:#type_name}
    }
    fn to_member_ref(&self, s: &str) -> TokenStream {
        let s = syn::parse_str::<syn::Type>(&s).unwrap();
        let member = syn::parse_str::<syn::Type>(&self.name).unwrap();
        quote! {
            &#s.#member
        }
    }

    fn to_read_from_row(&self, row: &str, index: usize) -> TokenStream {
        let member = syn::parse_str::<syn::Type>(&self.name).unwrap();
        let row = syn::parse_str::<syn::Type>(row).unwrap();
        quote! {
            #member:#row.get(#index)?
        }
    }
}

pub fn tagged_sql(struct_name_str: &str, sql: &str) -> proc_macro2::TokenStream {
    let r = Regex::new(r#"([ ]*[a-zA-Z0-9_]+)|([ ]*/\*:[^*]+\*/)"#).unwrap();

    let v: Vec<_> = r
        .captures_iter(sql)
        .map(|c| c.get(0).unwrap().as_str())
        .collect();
    let struct_name = syn::parse_str::<syn::Type>(&struct_name_str).unwrap();
    let param_name = syn::parse_str::<syn::Type>(&format!("{}Params", struct_name_str)).unwrap();
    let row_name = syn::parse_str::<syn::Type>(&format!("{}Row", struct_name_str)).unwrap();
    let mut select_members = Vec::new();
    let mut param_members = Vec::new();
    for (index, &part) in v.iter().enumerate() {
        let part = part.trim();
        if part.starts_with("/*:") && part.ends_with("*/") {
            let middle = part
                .strip_prefix("/*:")
                .unwrap()
                .strip_suffix("*/")
                .unwrap()
                .trim();
            if middle.find(":").is_none() {
                let name = v[index - 1].trim();
                select_members.push(SqlMember {
                    name: name.to_owned(),
                    type_name: middle.to_owned(),
                });
            } else {
                let parts: Vec<_> = middle.split(":").collect();
                let name = parts[0].trim();
                param_members.push(SqlMember {
                    name: name.to_owned(),
                    type_name: parts[1].to_owned(),
                });
            }
        }
    }

    let select_decls: Vec<_> = select_members.iter().map(|m| m.to_decl()).collect();

    let mut tokens = Vec::new();

    tokens.push(quote! {
    #[derive(PartialOrd, PartialEq, Debug)]
    struct #row_name{
        #(#select_decls),*
    }});

    let read_from_rows: Vec<_> = select_members
        .iter()
        .enumerate()
        .map(|(index, m)| m.to_read_from_row("row", index))
        .collect();

    tokens.push(quote! {
        impl tagged_rusqlite::TaggedRow for #row_name{
            fn sql_str() ->&'static str{
                #sql
            }
            fn from_row(row:&rusqlite::Row<'_>) -> rusqlite::Result<Self>
            where Self:Sized{
                Ok(Self{
                    #(#read_from_rows),*
                })
            }
        }
    });

    let param_decls: Vec<_> = param_members.iter().map(|m| m.to_decl()).collect();
    tokens.push(quote! {
    struct #param_name{
        #(#param_decls),*
    }});

    let param_refs: Vec<_> = param_members
        .iter()
        .map(|m| m.to_member_ref("self"))
        .collect();
    let mut binds = Vec::new();
    for (i, r) in param_refs.iter().enumerate() {
        let index = i + 1;
        binds.push(quote! {
            s.raw_bind_parameter(#index,#r)?;
        });
    }

    tokens.push(quote! {
        impl tagged_rusqlite::TaggedParams for #param_name{
            fn bind_all(&self,s:&mut rusqlite::Statement) ->rusqlite::Result<()>{
                #(#binds)*
                Ok(())
            }
        }
    });

    tokens.push(quote! {
        struct #struct_name{}
    });

    tokens.push(quote! {
        impl tagged_rusqlite::TaggedQuery for #struct_name{
            type Row = #row_name;
            type Params = #param_name;
                fn sql_str() ->&'static str{
                    #sql
                }
        }
    });

    tokens.push(quote! {
        impl<'a> #struct_name{
            pub fn prepare(connection: &'a rusqlite::Connection)->tagged_rusqlite::StatementHolder<'a,#struct_name>{
                tagged_rusqlite::StatementHolder::new(connection)
            }
        }
    });

    let struct_trait = if param_members.is_empty() {
        syn::parse_str::<syn::Type>("NoParams").unwrap()
    } else {
        syn::parse_str::<syn::Type>("HasParams").unwrap()
    };

    tokens.push(quote! {
        impl tagged_rusqlite::#struct_trait for #struct_name{}
    });

    quote! {#(#tokens)*}
}
