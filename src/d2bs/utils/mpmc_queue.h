#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace d2bs {

template <typename T>
class mpmc_queue {
 public:
  void enqueue(T&& item) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    pop_cv_.wait(lock, [this] { return queue_.size() < 100; });
    queue_.push(std::move(item));
    push_cv_.notify_one();
  }

  void enqueue_nowait(T&& item) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_.push(std::move(item));
    push_cv_.notify_one();
  }

  bool dequeue_for(T& popped_item, std::chrono::milliseconds wait_duration) {
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (!push_cv_.wait_for(lock, wait_duration, [this] { return !queue_.empty(); })) return false;

      popped_item = std::move(queue_.front());
      queue_.pop();
    }
    pop_cv_.notify_one();
    return true;
  }

  size_t size() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return queue_.size();
  }

 private:
  std::mutex queue_mutex_;
  std::condition_variable push_cv_;
  std::condition_variable pop_cv_;
  std::queue<T> queue_;
};

}  // namespace d2bs
