#include "shape_drawer.hpp"

void draw_shapes(const std::vector<std::unique_ptr<shape>>& shapes) {
	for (const auto& shape : shapes) shape->draw();
}
