#include <iostream>

struct circle {
	void draw() const {
		std::cout << "circle::draw\n";
	}
};

struct square {
	void draw() const {
		std::cout << "square::draw\n";
	}
};

template<typename Shape>
void draw(const Shape& shape) {
	shape.draw();
}

struct point {};

void draw(point) {
	std::cout << "point::draw\n";
}

int main() {
	square s;
	circle c;
	point p;

	draw(s);
	draw(c);
	draw(p);



}