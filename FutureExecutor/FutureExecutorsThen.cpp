
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <thread>
#include <iostream>

template <typename T>
class mtq {
 public:
  mtq(std::size_t max_size) : max_size_(max_size) {}

  void push(T t) {
    std::unique_lock<std::mutex> lock{mut_};
    for (;;) {
      if (q_.size() < max_size_) {
        q_.push(std::move(t));
        cvar_.notify_all();
        return;
      } else {
        cvar_.wait(lock);
      }
    }
  }

  std::optional<T> pop() {
    std::unique_lock<std::mutex> lock{mut_};
    for (;;) {
      if (!q_.empty()) {
        T t = q_.front();
        q_.pop();
        cvar_.notify_all();
        return t;
      } else {
        if (done_) return std::nullopt;
        cvar_.wait(lock);
      }
    }
  }

  bool done() const {
    std::unique_lock<std::mutex> lock{mut_};
    return done_;
  }

  void set_done() {
    std::unique_lock<std::mutex> lock{mut_};
    done_ = true;
    cvar_.notify_all();
  }

 private:
  std::size_t max_size_ = 0;
  bool done_ = false;
  std::queue<T> q_;
  mutable std::mutex mut_;
  mutable std::condition_variable cvar_;
};

#include <functional>

class thread_pool {
 public:
  thread_pool() : q_(std::numeric_limits<int>::max() - 1) { add_thread(); }
  ~thread_pool() {
    q_.set_done();
    for (auto& thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void add(std::function<void()> f) { q_.push(std::move(f)); }

  void add_thread() {
    std::unique_lock lock{mut_};
    threads_.emplace_back([this]() mutable {
      while (true) {
        auto f = q_.pop();
        if (f) {
          (*f)();
        } else {
          if (q_.done()) return;
        }
      }
    });
  }

 private:
  mtq<std::function<void()>> q_;
  std::mutex mut_;
  std::vector<std::thread> threads_;
};


template <typename T>
struct shared {
  T value;
  std::exception_ptr eptr = nullptr;
  std::mutex mutex;
  std::condition_variable cvar;
  bool done = false;
  std::function<void()> then;
  std::shared_ptr<thread_pool> pool;
};


template <typename T>
class future {
 public:
  void wait() {
    std::unique_lock<std::mutex> lock{shared_->mutex};
    while (!shared_->done) {
      shared_->cvar.wait(lock);
    }
  }
  template <typename F>
  auto then(F f) -> future<decltype(f(*this))>;

  T& get() {
    wait();
    if (shared_->eptr) {
      std::rethrow_exception(shared_->eptr);
    }
    return shared_->value;
  }

  explicit future(const std::shared_ptr<shared<T>>& shared) : shared_(shared) {}

 private:
  std::shared_ptr<shared<T>> shared_;
};

template<typename T>
void run_then(std::unique_lock<std::mutex> lock, std::shared_ptr<shared<T>>& s) {
  std::function<void()> f;
  if (s->done) {
    std::swap(f, s->then);
  }
  lock.unlock();
  if(f) f();
 
}

template <typename T>
class promise {
 public:
 promise():shared_(std::make_shared<shared<T>>()){}
 template<typename V>
  void set_value(V&& v) {

    std::unique_lock<std::mutex> lock{shared_->mutex};
    shared_->value = std::forward<V>(v);
    shared_->done = true;
    run_then(std::move(lock), shared_);
    shared_->cvar.notify_one();
    shared_ = nullptr;
  }
  void set_exception(std::exception_ptr eptr) {
    std::unique_lock<std::mutex> lock{shared_->mutex};
    shared_->eptr = eptr;
    shared_->done = true;
    run_then(std::move(lock), shared_);
    shared_->cvar.notify_one();
    shared_ = nullptr;
  }

  future<T> get_future() { return future<T>{shared_}; }

  explicit promise(const std::shared_ptr<shared<T>>& shared)
      : shared_(shared) {}

 private:
  std::shared_ptr<shared<T>> shared_;
};



template <typename T>
template <typename F>
auto future<T>::then(F f) -> future<decltype(f(*this))> {
  std::unique_lock<std::mutex> lock{shared_->mutex};
  using type = decltype(f(*this));
  auto then_shared = std::make_shared<shared<type>>();
  then_shared->pool = shared_->pool;
  shared_->then = [shared = shared_,then_shared,f = std::move(f), pool = shared_->pool]() mutable {
    pool->add([shared,then_shared,f = std::move(f), p = promise<type>(then_shared)] ()mutable {
      future<T> fut(shared);
      p.set_value(f(fut));
    });
  };
  run_then(std::move(lock), shared_);
 return future<type>(then_shared);
}


template <typename F, typename... Args>
auto async(std::shared_ptr<thread_pool> pool, F f, Args... args) -> future<decltype(f(args...))> {
  using T = decltype(f(args...));
  auto state = std::make_shared<shared<T>>();
  state->pool = pool;
  promise<T> p(state);
  auto fut = p.get_future();
  auto future_func = [p = std::move(p), f = std::move(f), args...]() mutable {
    try {
        p.set_value(f(args...));
    } catch (...) {
      p.set_exception(std::current_exception());
    }
  };
  pool->add(std::move(future_func));
 return fut;
} 

#include <iostream>
int main() {
  auto pool = std::make_shared<thread_pool>();
  pool->add([](){
    std::cout << "Hi from thread pool\n";
  });
  auto f = async(pool, []() {
    std::cout << "Hello\n";
    return 1;
  });
  auto f2 = f.then([](auto& f) {
    std::cout << f.get() << " World\n";
    return 2.0;
  });
  f2.get();
}
