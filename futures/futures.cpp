#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <variant>

template <class... T>
struct overload : T... {
  using T::operator()...;
};

template <typename... T>
overload(T... t)->overload<T...>;

template <typename T>
struct shared {
  T value;
  std::exception_ptr eptr = nullptr;
  std::mutex mutex;
  std::condition_variable cvar;
  bool done = false;
  std::variant<std::thread, std::function<void()>> thread_or_function;
};

template <>
struct shared<void> {
  std::exception_ptr eptr = nullptr;
  std::mutex mutex;
  std::condition_variable cvar;
  bool done = false;
  std::variant<std::thread, std::function<void()>> thread_or_function;
};

template<typename T>
struct ref_not_void{
  using type = T&;
};
template<>
struct ref_not_void<void>{
  using type = void;
};

template<typename T>
using ref_not_void_t = typename ref_not_void<T>::type;


template <typename T>
class future {
 public:
  void wait() {
    std::unique_lock<std::mutex> lock{shared_->mutex};
    while (!shared_->done) {
      shared_->cvar.wait(lock);
    }
  }

  ref_not_void_t<T> get() {
    wait();
    if (shared_->eptr) {
      std::rethrow_exception(shared_->eptr);
    }
    std::visit(
        overload{[&](std::thread& t) { t.join(); },
        [&](auto& f) { f(); }},
        shared_->thread_or_function);
    if constexpr (!std::is_same_v<T, void>) {
      return shared_->value;
    }
  }

  explicit future(const std::shared_ptr<shared<T>>& shared) : shared_(shared) {}
 private:
  std::shared_ptr<shared<T>> shared_;
};
enum class launch { async, deferred };

template <typename T>
class promise {
 public:
 promise():shared_(std::make_shared<shared<T>>()){}
  void set_value() {
    shared_->done = true;
    shared_->cvar.notify_one();
    shared_ = nullptr;
  }
  template<typename V>
  void set_value(V&& v) {
    std::unique_lock<std::mutex> lock{shared_->mutex};
    shared_->value = std::forward<V>(v);
    shared_->done = true;
    shared_->cvar.notify_one();
    shared_ = nullptr;
  }
  void set_exception(std::exception_ptr eptr) {
    std::unique_lock<std::mutex> lock{shared_->mutex};
    shared_->eptr = eptr;
    shared_->done = true;
    shared_->cvar.notify_one();
    shared_ = nullptr;
  }

  future<T> get_future() { return future<T>{shared_}; }

  explicit promise(const std::shared_ptr<shared<T>>& shared)
      : shared_(shared) {}

 private:
  std::shared_ptr<shared<T>> shared_;
};

template <typename F, typename... Args>
auto async(launch policy, F f, Args... args) -> future<decltype(f(args...))> {
  using T = decltype(f(args...));
  auto state = std::make_shared<shared<T>>();
  promise<T> p(state);
  auto fut = p.get_future();
  auto future_func = [p = std::move(p), f = std::move(f), args...]() mutable {
    try {
      if constexpr (std::is_same_v<T, void>) {
        f(args...);
        p.set_value();
      } else {
        p.set_value(f(args...));
      }
    } catch (...) {
      p.set_exception(std::current_exception());
    }
  };
  if (policy == launch::async) {
    state->done = false;
    state->thread_or_function = std::thread(std::move(future_func));
  } else {
    state->done = true;
    state->thread_or_function = std::move(future_func);
  }
  return fut;
}

#include <iostream>

int main() {
  auto fut = async(launch::async, []() { return 5; });
  std::cout << fut.get() << "\n";
  auto fut2 = async(launch::deferred, []() { return 5; });
  std::cout << fut2.get() << "\n";
  int v = 0;
  auto fut3 = async(launch::async, [&]()mutable{v = 4;});
  fut3.get();
  std::cout << v << "\n";
}