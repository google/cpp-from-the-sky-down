#include <iostream>
#include <string>
#include <type_traits>

using my_time = int;

template <typename T>
void draw(const T& t, std::ostream& os) {
  os << t << "\n";
}

template <typename T>
void debug_draw(const T& t, std::ostream& os) {
  os << "Debug\n";
  draw(t, os);
}

// By default do nothing
template <typename T>
void update(T& t, my_time time) {}

struct my_clock {
  my_time time = 0;
  void update(my_time t) { time = t; }
};

inline void draw(const my_clock& c, std::ostream& os) {
  os << "The time is now: " << c.time << "\n";
}

inline void update(my_clock& c, my_time time) { c.update(time); }


template<typename T>
void draw_helper(const T& t){
  draw(t, std::cout);
}

int main() {
  int a = 5;
  std::string s = "Hello";
  my_clock c;

  update(a, 1);
  update(s, 1);
  update(c, 1);

  draw(a, std::cout);
  draw(s, std::cout);
  draw(c, std::cout);

  debug_draw(a, std::cout);
  debug_draw(s, std::cout);
  debug_draw(c, std::cout);

  draw_helper(c);
}
