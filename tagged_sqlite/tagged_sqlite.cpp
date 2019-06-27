// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"

using namespace std;

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

  auto query =
      query_builder<db>()
          .select(column<customers, id>, column<orders, customerid>,
                  column<name>, column<orders, id>.as<class orderid>(),
                  column<price>)
          .from(table<customers>, table<orders>)
          .join(column<orderid> == column<customerid>)
          .where(column<price>> parameter<class price_parameter, double>())
          .build();

  // for (auto& row : execute_query(query, parameter<price_parameter>(100.0))) {
  //  std::cout << row.get<customers, id>();
  //  std::cout << row.get<name>();
  //}

  cout << to_statement(query) << endl;

  auto e = column<class Test> == constant(5) ||
           column<class A> * constant(5) <= parameter<class P1, std::int64_t>();

  cout << expression_to_string(e) << std::endl;

  cout << "\n"
       << simple_type_name::long_name<
              detail::column_type<db, item>::value_type> << "\n";

  return 0;
}
