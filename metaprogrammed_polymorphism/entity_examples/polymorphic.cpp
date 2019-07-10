#include "..//polymorphic.hpp"
#include <iostream>
#include <memory>

using my_time = int;

class draw;
class debug_draw;
class update;

template <typename T> void poly_extend(draw *, const T &t, std::ostream &os) {
  os << t << "\n";
}

template <typename T>
void poly_extend(debug_draw *, const T &t, std::ostream &os) {
  os << "Debug\n";
  poly_extend(static_cast<draw *>(nullptr), t, os);
}

// By default do nothing
template <typename T> void poly_extend(update *, T &t, my_time time) {}

using entity = polymorphic::object<void(draw, std::ostream &os) const,
                                   void(debug_draw, std::ostream &os) const,
                                   void(update, my_time t)>;

struct my_clock {
  my_time time = 0;
  void update(my_time t) { time = t; }
};

inline void poly_extend(draw *, const my_clock &c, std::ostream &os) {
  os << "The time is now: " << c.time << "\n";
}

inline void poly_extend(update *, my_clock &c, my_time time) { c.update(time); }

#include <string>
#include <vector>

inline void
draw_helper(polymorphic::ref<void(draw, std::ostream &os) const> d) {
  d.call<draw>(std::cout);
}

int main() {
  std::vector<entity> entities{5, std::string("Hello"), my_clock()};
  auto entities2 = entities;
  for (auto &e : entities) {
    e.call<update>(1);
    e.call<draw>(std::cout);
  }
  for (const auto &e : entities2) {
    e.call<draw>(std::cout);
  }
  for (const auto& e : entities) {
	  draw_helper(e);
  }
}
