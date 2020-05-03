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
  skydown::check_sqlite_return(rc);

  rc = sqlite3_exec(sqldb, insertion, 0, 0, nullptr);
  skydown::check_sqlite_return(rc);

  return sqldb;
}

template <typename... Fields>
std::string field_names() {
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

inline std::string get_name_from_user() { return "John"; }

int main() {
  sqlite3 *sqldb = init_database();
  static constexpr std::string_view select_sql =
      R"(
SELECT  <:name:string>,  <:item:string?>, <:price:double>
FROM orders JOIN customers ON customers.id = customerid where price > {:price:double}

)";

  static constexpr std::string_view insert_sql = R"(
INSERT INTO orders( item , customerid , price ) VALUES ({:item:string},{:customerid:int} ,{:price:double});
)";
  class customers;
  class id;
  class item;
  class name;
  class customerid;
  class price;
  using skydown::sqlite_experimental::fld;
  using skydown::sqlite_experimental::parm;

  auto insert_statement =
      skydown::sqlite_experimental::prepare_query_string<insert_sql>(sqldb);
  auto r = insert_statement.execute(parm<customerid>(1), parm<price>(2000),
                                    parm<item>("MacBook"));
  assert(r);

  auto select_statement =
      skydown::sqlite_experimental::prepare_query_string<select_sql>(sqldb);
  auto rows = select_statement.execute_rows(parm<price>(200));
  assert(!rows.has_error());

  for (auto &row : rows) {
    std::cout << fld<price>(row) << " ";
    std::cout << fld<name>(row) << " ";
    std::cout << fld<item>(row).value() << "\n";
  }
  auto rows2 = select_statement.execute_rows(parm<price>(100));
  assert(!rows2.has_error());
  for (auto &row : rows2) {
    std::cout << fld<price>(row) << " ";
    std::cout << fld<name>(row) << " ";
    std::cout << fld<item>(row).value() << "\n";
  }
}
