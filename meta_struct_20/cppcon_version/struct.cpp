#include <string>
#include <iostream>

struct person {
  int id = 1;
  std::string name;
  int score = 0;
};

int main() {
  person p{.id = 1, .name = "John"};
  p.id = 2;
  p.name = "JRB";
  std::cout << p.id << " " << p.name;
}