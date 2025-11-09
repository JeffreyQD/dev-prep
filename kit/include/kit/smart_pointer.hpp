#ifndef __KIT_SHARED_PTR_BASE_HPP__
#define __KIT_SHARED_PTR_BASE_HPP__

#include <atomic>

namespace kit {

struct Counter {
  Counter(int use_count_, int weak_cnt_)
      : use_count(use_count_), weak_cnt(weak_cnt_) {}

  std::atomic<int> use_count;
  std::atomic<int> weak_cnt;
};

template <typename T>
class weak_ptr;

template <typename T>
class shared_ptr {
  friend class weak_ptr<T>;

 public:
  shared_ptr(T* p) : data_(p) {
    if (data_) {
      cnt_ = new Counter(1, 1);
    }
  }
  shared_ptr() : data_(nullptr), cnt_(nullptr) {}
  ~shared_ptr() { dispose(); }

  shared_ptr(const shared_ptr& other) {
    if (other.data_) {
      data_ = other.data_;
      cnt_ = other.cnt_;
      cnt_->use_count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  shared_ptr& operator=(const shared_ptr& other) {
    if (this != &other) {
      dispose();
      data_ = other.data_;
      cnt_ = other.cnt_;
      cnt_->use_count.fetch_add(1, std::memory_order_relaxed);
    }
    return *this;
  }

  shared_ptr(shared_ptr&& other) {
    std::cout << "move constructor\n";
    if (other.data_) {
      data_ = other.data_;
      cnt_ = other.cnt_;
      other.data_ = nullptr;
      other.cnt_ = nullptr;
    }
  }

  shared_ptr& operator=(shared_ptr&& other) {
    std::cout << "move assignment\n";
    if (this != &other) {
      std::swap(data_, other.data_);
      std::swap(cnt_, other.cnt_);
    }
    return *this;
  }

  long use_count() const noexcept {
    return cnt_ ? cnt_->use_count.load(std::memory_order_relaxed) : 0;
  }

  explicit operator bool() const noexcept { return data_; }

  T& operator*() { return *data_; }

  T* operator->() { return data_; }

 private:
  void dispose() {
    int use_count = cnt_->use_count.fetch_sub(1, std::memory_order_relaxed);
    if (use_count == 1) {
      delete data_;
      int weak_cnt = cnt_->weak_cnt.fetch_sub(1, std::memory_order_relaxed);
      if (weak_cnt == 0) {
        delete cnt_;
      }
    }
  }

  T* data_;
  Counter* cnt_;
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  T* p = new T(std::forward<Args>(args)...);
  return shared_ptr<T>(p);
}

template <typename T>
class weak_ptr {
 public:
  weak_ptr() : cnt_(nullptr) {}
  weak_ptr(const shared_ptr<T>& other) {
    data_ = other.data_;
    cnt_ = other.cnt_;
    if (cnt_) {
      cnt_->weak_cnt.store(cnt_->use_count.load(std::memory_order_relaxed) + 1,
                           std::memory_order_relaxed);
    }
  }

  /**
   * * Returns the number of shared_ptr instances that share the
   * * ownership of the managed object.
   */
  long use_count() const noexcept {
    return cnt_ ? cnt_->use_count.load(std::memory_order_relaxed) : 0;
  }
  bool expired() const noexcept { return use_count() == 0; }

  shared_ptr<T> lock() {
    shared_ptr<T> ret;
    if (!expired()) {
      ret.data_ = data_;
      ret.cnt_ = cnt_;
      cnt_->use_count.fetch_add(1, std::memory_order_relaxed);
    }
    return ret;
  }

 private:
  T* data_;
  Counter* cnt_;
};

}  // namespace kit

#endif