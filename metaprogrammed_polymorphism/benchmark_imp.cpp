#include "benchmark_imp.h"

struct Imp :Base {
	int draw() override { return 5; }

};



std::unique_ptr<Base> MakeBase() {
	return std::make_unique<Imp>();
}
int poly_extend(draw*, Dummy&) { return 5; }


std::function<int(Dummy&)> GetFunction() {
	return [](Dummy&) {return 5; };
}

int NonVirtual::draw() { return 5; }
