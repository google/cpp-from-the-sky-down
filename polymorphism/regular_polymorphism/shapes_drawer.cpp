#include "shapes_drawer.hpp"

void draw_shapes(const std::vector<my_shapes::shape>& shapes) {
	for (const auto& shape : shapes) shape.draw();
}
