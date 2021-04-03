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

use tagged_rusqlite_proc_impl::tagged_sql;

fn main() {
    println!(
        "{}",
        tagged_sql("MyStruct", "Select name /*:String*/ ? /*:user:i32*/")
    );
    println!("{:?}", tagged_sql("MyStruct", "Select name /*:String*/"));
}
