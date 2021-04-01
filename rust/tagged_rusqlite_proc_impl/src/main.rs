use tagged_rusqlite_proc_impl::tagged_sql;

fn main() {
    println!("{}",tagged_sql("MyStruct","Select name /*:String*/ ? /*:user:i32*/"));
    println!("{:?}",tagged_sql("MyStruct","Select name /*:String*/"));
}
