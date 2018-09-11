#pragma once
#include "shapes_interface.hpp"

struct circle {
	void draw() const { std::cout << "circle::draw\n"; }
};

struct square {
	void draw() const { std::cout << "square::draw\n"; }
};

struct composite {
	std::vector<my_shapes::shape> shapes_;
	void draw() const {
		std::cout << "begin composite\n";
		for (auto& s : shapes_) s.draw();
		std::cout << "end composite\n";
	}
};

namespace other_library {
	struct triangle {
		void display()const { std::cout << "triangle::display\n"; }
	};

	void draw_implementation(const triangle& t) { t.display(); }

}

namespace my_namespace {
	struct my_shape {};
	void draw_implementation(my_shape) { std::cout << "my_shape:draw\n"; }
}  // namespace my_namespace

