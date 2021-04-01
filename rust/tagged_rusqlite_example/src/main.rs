use tagged_rusqlite_proc::tagged_sql;
fn main() {
    tagged_sql!("Hello");
    println!("Hello, world! {}", Hello::get_str());
}
