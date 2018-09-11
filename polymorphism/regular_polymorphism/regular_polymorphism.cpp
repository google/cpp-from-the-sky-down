#include <iostream>
#include <memory>
#include <vector>
#include "shapes.hpp"
#include "shapes_drawer.hpp"

int main() {
	std::vector<my_shapes::shape> shapes;
	shapes.emplace_back();
	shapes.emplace_back(circle{});
	shapes.emplace_back(square{});
	composite c;
	c.shapes_.emplace_back(square{});
	c.shapes_.emplace_back(circle{});
	shapes.emplace_back(std::move(c));
	shapes.emplace_back(other_library::triangle{});
	shapes.emplace_back(my_namespace::my_shape{});

	draw_shapes(shapes);


}
