#include <iostream>
#include <memory>
#include <vector>

using my_time = int;

struct draw_interface {
  virtual void draw(std::ostream& os) const = 0;
  virtual ~draw_interface() {}
};
struct entity_interface : draw_interface {
  virtual void debug_draw(std::ostream& os) const = 0;
  virtual void update(my_time t) = 0;
};

struct int_holder : entity_interface {
  void draw(std::ostream& os) const override { os << value << "\n"; }
  void debug_draw(std::ostream& os) const override {
    os << "Debug\n" << value << "\n";
  }
  void update(my_time t) override {}

  int_holder(int v) : value(v) {}
  int value;
};

struct my_clock : entity_interface {
  my_time time = 0;
  void draw(std::ostream& os) const override {
    os << "The time is: " << time << "\n";
  }
  void debug_draw(std::ostream& os) const override {
    os << "Debug\n" << time << "\n";
  }
  void update(my_time t) override { time = t; }
};

void draw_helper(const draw_interface& d) { d.draw(std::cout); }

int main() {
  std::vector<std::unique_ptr<entity_interface>> entities;

  entities.push_back(std::make_unique<int_holder>(5));
  entities.push_back(std::make_unique<my_clock>());

  for (auto& e : entities) {
    draw_helper(*e);
  }
}
