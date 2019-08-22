// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"
#include <exception>

using namespace std;

void check_sqlite_return(int r) {
	if (r != SQLITE_OK) {
		std::cerr << "SQLITE ERROR " << r;
		throw std::runtime_error("sqlite error");
	}
}

int main()
{
  using db = define_database<
      define_table<class customers, //
                   define_column<class id, std::int64_t>,
                   define_column<class name, std::string>>,
      define_table<class orders, //
                   define_column<class id, std::int64_t>,
                   define_column<class item, std::string>,
                   define_column<class customerid, std::int64_t>,
                   define_column<class price, double>>>;

  auto query = query_builder<db>()
                   .select(column<customers, id>, column<customerid>,
                           column<customers,name>, column<orders, id>,
                           column<price>)
                   .from(join(table<customers>, table<orders>,
                              column<orders,id> == column<customerid>))
                   .where(column<price> > val(2.0))
                   .build();
  // for (auto& row : execute_query(query, parameter<price_parameter>(100.0))) {
  //  std::cout << row.get<customers, id>();
  //  std::cout << row.get<name>();
  //}
  std::cout <<(query);

  sqlite3* sqldb;
  sqlite3_open(":memory:", &sqldb);

  auto const creation = R"(
CREATE TABLE customers(id PRIMARY_KEY, name TEXT);
CREATE TABLE orders(id PRIMARY_KEY, item TEXT, customerid INTEGER, price REAL);
)";

  auto const insertion = R"(
INSERT INTO customers(id, name) VALUES(1, "John");
INSERT INTO orders(id,  item , customerid , price ) VALUES(1,"Laptop",1,122.22);
)";

  int rc;
  rc = sqlite3_exec(sqldb, creation, 0, 0, nullptr);
  check_sqlite_return(rc);

  rc = sqlite3_exec(sqldb, insertion, 0, 0, nullptr);
  check_sqlite_return(rc);

  sqlite3_stmt* stmt;
  auto query_string = to_statement(query);
  rc = sqlite3_prepare_v2(sqldb, query_string.c_str(), -1, &stmt, 0);
  check_sqlite_return(rc);
  rc = sqlite3_bind_double(stmt, 1, 2.0);
  check_sqlite_return(rc);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW) {
	  std::cerr << "ERROR IN STEP\n";
	  return -1;
  }

  auto row = read_row(query,stmt);
  

  cout << to_statement(query) << endl;


  std::cout << field<price>(row).value();

  //  cout << query << endl;
  return 0;
  auto p1 = process<db>(parameter<class price_parameter, double>(),
                        tagged_tuple::make_ttuple());
  cout << p1 << endl;
  auto p2 = process<db>(val("a") == column<orders, item>, p1);
  cout << p2 << endl;
  cout << process<db>(val(4), p2) << endl;

  auto e = column<class Test> == val(5) ||
           column<class A> * val(5) <= parameter<class P1, std::int64_t>();

  cout << expression_to_string(e) << std::endl;

  cout << "\n"
       << simple_type_name::long_name<
              detail::column_type<db, item>::value_type> << "\n";

  return 0;
}
