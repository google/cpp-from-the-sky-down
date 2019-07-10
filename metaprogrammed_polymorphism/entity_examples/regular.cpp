#include <iostream>
#include <memory>

using my_time = int;

template <typename T> void draw(const T &t, std::ostream &os) {
  os << t << "\n";
}

template <typename T> void debug_draw(const T &t, std::ostream &os) {
  os << "Debug\n";
  draw(t, os);
}

// By default do nothing
template <typename T> void update(T &t, my_time time) {}

namespace detail {
struct entity_interface {
  virtual void draw_(std::ostream &os) const = 0;
  virtual void debug_draw_(std::ostream &os) const = 0;
  virtual void update_(my_time t) = 0;
  virtual entity_interface *clone_() const = 0;
  virtual ~entity_interface() {}
};

template <typename T> struct entity_imp : entity_interface {
  void draw_(std::ostream &os) const override { draw(t_, os); }
  void debug_draw_(std::ostream &os) const override { debug_draw(t_, os); }
  void update_(my_time t) override { update(t_, t); }
  entity_interface *clone_() const { return new entity_imp(t_); }

  entity_imp(T t) : t_(std::move(t)) {}

  T t_;
};
} // namespace detail

struct entity {
  template <typename T>
  entity(T t) : e_(std::make_unique<detail::entity_imp<T>>(std::move(t))) {}
  entity(entity &&) = default;
  entity &operator=(entity &&) = default;

  entity(const entity &other) : e_(other.e_ ? other.e_->clone_() : nullptr) {}
  entity &operator=(const entity &other) {
    entity tmp(other);
    *this = std::move(tmp);
    return *this;
  }

  explicit operator bool() const { return e_ != nullptr; }
  void draw(std::ostream &os) const { e_->draw_(os); }
  void debug_draw(std::ostream &os) const { e_->debug_draw_(os); }
  void update(my_time t) { e_->update_(t); }

  std::unique_ptr<detail::entity_interface> e_;
};

struct my_clock {
  my_time time = 0;
  void update(my_time t) { time = t; }
};

inline void draw(const my_clock &c, std::ostream &os) {
  os << "The time is now: " << c.time << "\n";
}

inline void update(my_clock &c, my_time time) { c.update(time); }

#include <string>
#include <vector>
int main() {
  std::vector<entity> entities{5, std::string("Hello"), my_clock()};
  auto entities2 = entities;
  for (auto &e : entities) {
    e.update(1);
	e.draw(std::cout);
  }
  for (const auto &e : entities2) {
    e.draw(std::cout);
  }
}
