// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"

using namespace std;

int main() {
  using qb = query_builder<mydb>;
  auto ss = qb().select(column<id>, column<customers, customerid>,
                       column<orders, id>.as<class orderid>()) 
                .from(table<customers>, table<orders>);
  cout << to_statement(ss) << endl;

  auto e = column<class Test> == 5 || column<class A> *5 <= parameter<class P1>();

  cout << expression_to_string(e) << std::endl;

  return 0;
}
