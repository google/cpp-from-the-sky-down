#pragma once
#include <iostream>
#include "shapes_interface.hpp"

struct square :public shape {
	void draw() const {
		std::cout << "square::draw\n";
	}
};

struct circle :public shape {
	void draw() const {
		std::cout << "circle::draw\n";
	}
};

