// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"
#include <exception>


sqlite3 *init_database() {
  sqlite3 *sqldb;
  sqlite3_open(":memory:", &sqldb);

  auto const creation = R"(
CREATE TABLE customers(id PRIMARY_KEY, name TEXT);
CREATE TABLE orders(id PRIMARY_KEY, item TEXT, customerid INTEGER, price REAL);
)";

  auto const insertion = R"(
INSERT INTO customers(id, name) VALUES(1, "John");
INSERT INTO orders(id,  item , customerid , price ) VALUES (1,"Laptop",1,122.22), (2,"Phone",1,444.44);
)";

  int rc;
  rc = sqlite3_exec(sqldb, creation, 0, 0, nullptr);
  check_sqlite_return(rc);

  rc = sqlite3_exec(sqldb, insertion, 0, 0, nullptr);
  check_sqlite_return(rc);

  return sqldb;
}

template <typename... Fields> std::string field_names() {
  auto s = (... + (std::string(simple_type_name::short_name<Fields>) + "."));
  if (s.size() > 0) {
    s.pop_back();
  }
  return s;
}

template <typename... Fields, typename Row>
std::ostream &print_field(std::ostream &os, const Row &row) {
  os << field_names<Fields...>() << ": ";
  auto &val = field<Fields...>(row);
  if (val.has_value()) {
    os << val.value();
  } else {
    os << "NULL";
  }
  os << "\n";
  return os;
}

int main() {
  using db = define_database<
      define_table<class customers, //
                   define_column<class id, std::int64_t>,
                   define_column<class name, std::string>>,
      define_table<class orders, //
                   define_column<class id, std::int64_t>,
                   define_column<class item, std::string>,
                   define_column<class customerid, std::int64_t>,
                   define_column<class price, double>>>;

  query_t query = select<db>(column<customers, id>, column<customerid>,
                           column<customers, name>, column<orders, id>,
                           column<orders, item>, column<price>)
                   .from(join(table<orders>, table<customers>,
                              column<customers, id> == column<customerid>))
                   .where(column<price> < 200.0 && column<orders,id> * 5 + 1 < 600 &&
                          column<customers, name> == "John");
                   
  auto sqldb = init_database();
  for (auto &row : execute_query(query, sqldb)) {
    std::cout << "{\n";
	// Access using field
	auto& v = field<customers, name>(row);
	std::cout << "Customer Name:" << v.value_or("NULL") << "\n";
	// Use the convenience print_field function.
    print_field<customers, name>(std::cout, row);
    print_field<orders, id>(std::cout, row);
    print_field<price>(std::cout, row);
    std::cout << "}\n";
  }
  std::cout << "The sql statement is:\n" << to_statement(query.t_) << "\n";
  std::cout << "\nThe parameters to the query are:\n";
  std::cout << tagged_tuple::get<expression_parts::arguments>(query.t_);
  return 0;
}
