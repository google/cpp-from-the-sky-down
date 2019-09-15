#include <memory>
#include <functional>
#include "polymorphic.hpp"
struct Dummy {};

class draw {};
int poly_extend(draw, Dummy&);

struct Base {
	virtual int draw() = 0;
	virtual ~Base() {}
};

struct NonVirtual {

	int draw();
};

std::function<int()> GetFunction();
std::unique_ptr<Base> MakeBase();

std::function<int()> GetFunctionRand(int r);
std::unique_ptr<Base> MakeBaseRand(int r);
polymorphic::object<int(draw)> GetObjectRand(int r);