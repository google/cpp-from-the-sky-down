#include "tagged_struct.h"

#include <iostream>
int main(){
using namespace literals;

tagged_struct ts{"hello"_tag = 1};

std::cout << ts->*"hello"_tag << "\n";

}