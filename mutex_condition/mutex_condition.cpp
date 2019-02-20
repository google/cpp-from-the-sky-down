#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <thread>

template <typename T>
class mtq {
 public:
  mtq(int max_size) : max_size_(max_size) {}

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
  int max_size_ = 0;
  bool done_ = false;
  std::queue<T> q_;
  mutable std::mutex mut_;
  mutable std::condition_variable cvar_;
};

class latch {
 public:
  latch(int counter) : counter_(counter) {}

  void wait() {
    std::unique_lock<std::mutex> lock{mut_};
    for (;;) {
      if (counter_ <= 0) {
        return;
      } else {
        cvar_.wait(lock);
      }
    }
  }

  void count_down(std::ptrdiff_t n = 1) {
    std::unique_lock<std::mutex> lock{mut_};
    if (counter_ > 0) {
      counter_ -= n;
    }
    cvar_.notify_all();
  }

 private:
  int counter_ = 0;
  mutable std::mutex mut_;
  mutable std::condition_variable cvar_;
};

class thread_group {
 public:
  template <typename... Args>
  void push(Args &&... args) {
    threads_.emplace_back(std::forward<Args>(args)...);
  }

  void join_all() {
    for (auto &thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  ~thread_group() { join_all(); }

 private:
  std::vector<std::thread> threads_;
};

std::vector<char> compress(const std::vector<char> &in) {
  std::this_thread::sleep_for(std::chrono::seconds(rand() % 5 + 1));
  return in;
}

std::vector<char> read(std::istream &is, size_t n) {
  std::vector<char> in(n);
  if (!is.read(in.data(), n)) {
    in.resize(is.gcount());
  }
  return in;
}

void write(std::ostream &os, const std::vector<char> &out) {
  os.write(out.data(), out.size());
}

using block_q = mtq<std::pair<int, std::vector<char>>>;

void reader(block_q &reader_q, mtq<int> &back_pressure, std::istream &is) {
  int id = 0;
  while (is) {
    back_pressure.pop();
    reader_q.push({id, read(is, 3)});
    ++id;
  }
  reader_q.set_done();
}

void compressor(block_q &reader_q, block_q &writer_q, latch &l) {
  for (;;) {
    auto block = reader_q.pop();
    if (!block) {
      l.count_down();
      return;
    }
    writer_q.push({block->first, compress(block->second)});
  }
}

void writer_q_closer(block_q &writer_q, latch &l) {
  l.wait();
  writer_q.set_done();
}

struct in_order_comparator {
  template <typename T>
  bool operator()(T &a, T &b) {
    return a.first > b.first;
  }
};
void in_order_writer(block_q &writer_q, mtq<int> &back_pressure,
                     std::ostream &os) {
  int counter = 0;
  std::priority_queue<std::pair<int, std::vector<char>>,
                      std::vector<std::pair<int, std::vector<char>>>,
                      in_order_comparator>
      pq;
  for (;;) {
    while (!pq.empty() && pq.top().first == counter) {
      write(os, pq.top().second);
      back_pressure.push(1);
      ++counter;
      pq.pop();
    }
    auto block = writer_q.pop();
    if (!block) return;
    pq.push(std::move(*block));
  }
}

#include <iostream>
int main() {
  thread_group tg;
  std::string instring = "abcdefghijklmnopqrstuvwxyz";
  std::istringstream is(instring);
  std::ostringstream os;
  const int q_depth = 15;
  block_q reader_q(q_depth);
  block_q writer_q(q_depth);
  mtq<int> back_pressure(q_depth);
  for (int i = 0; i < q_depth; ++i) {
    back_pressure.push(1);
  }
  latch l(std::thread::hardware_concurrency());
  tg.push([&]() mutable { reader(reader_q, back_pressure, is); });
  for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    tg.push([&]() mutable { compressor(reader_q, writer_q, l); });
  }
  tg.push([&]() mutable { writer_q_closer(writer_q, l); });
  tg.push([&]() mutable { in_order_writer(writer_q, back_pressure, os); });
  tg.join_all();

  if (os.str() == instring) {
    std::cout << "SUCCESS\n";
  } else {
    std::cout << "FAILURE\n";
  }
  std::cout << os.str() << "\n";
}
