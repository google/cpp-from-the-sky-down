// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"

using namespace std;

int main() {

  using db = define_database<
      class mydb,
      define_table<class customers, define_column<class id, std::int64_t>,
                   define_column<class name, std::string>>,
      define_table<class orders, define_column<class id, std::int64_t>,
                   define_column<class item, std::string>,
                   define_column<class customerid, std::int64_t>>>;

  using qb = query_builder<mydb>;
  auto ss = qb().select(column<id>, column<customers, customerid>,
                        column<orders, id>.as<class orderid>())
                .from(table<customers>, table<orders>);
  cout << to_statement(ss) << endl;

  auto e = column<class Test> == constant(5) ||
           column<class A> * constant(5) <= parameter<class P1, std::int64_t>();

  cout << expression_to_string(e) << std::endl;


     
  cout << "\n" << simple_type_name::long_name<detail::column_type<db,item>::value_type>  << "\n";

  return 0;
}
