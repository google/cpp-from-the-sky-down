// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"

using namespace std;

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
                   .select(column<customers, id>, column<orders, customerid>,
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

  return 0;

  cout << to_statement(query) << endl;
  //  cout << query << endl;
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
