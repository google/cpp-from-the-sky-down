#include <vector>
#include <memory>
#include "shapes.hpp"
#include "shape_drawer.hpp"


int main() {
	
	std::vector<std::unique_ptr<shape>> shapes;

	shapes.push_back(std::make_unique<circle>());
	shapes.push_back(std::make_unique<square>());

	draw_shapes(shapes);


}