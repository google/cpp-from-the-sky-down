// tagged_sqlite.cpp : Defines the entry point for the application.
//

#include "tagged_sqlite.h"

using namespace std;

int main() {
  using qb = query_builder<mydb>;
  auto ss = qb().select(column<id>, column<customers, customerid>,
                       column<orders, id>.as<class orderid>) 
                .from(table<customers>, table<orders>);
  cout << to_statement(ss) << endl;
  return 0;
}
