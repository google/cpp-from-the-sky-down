use tagged_rusqlite_proc::tagged_sql;
fn main() {
    tagged_sql!(Hello,"Select name /*:String*/");
    println!("{}",Hello::sql_str());
}
