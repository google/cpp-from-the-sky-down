#include "benchmark_imp.h"

struct Imp :Base {
	int draw() override { return 5; }

};

struct Imp2 :Base {
	int draw() override { return 10; }

};



std::unique_ptr<Base> MakeBase() {
	return std::make_unique<Imp>();
}
int poly_extend(draw, Dummy&) { return 5; }
int poly_extend(draw, int&) { return 10; }


std::function<int()> GetFunction() {
	return []() {return 5; };
}

int NonVirtual::draw() { return 5; }

std::function<int()> GetFunctionRand(int r) {
	if (r % 2) {
		return []() {return 5; };
	}
	else {
		return []() {return 10; };
	}
}
std::unique_ptr<Base> MakeBaseRand(int r) {
	if (r % 2) {
		return std::make_unique<Imp2>();
	}
	else {
		return std::make_unique<Imp>();
	}
}
polymorphic::object<int(draw)> GetObjectRand(int r) {
	if (r % 2) {
		return { Dummy{} };

	}
	else {
		return { int{} };
	}


}
