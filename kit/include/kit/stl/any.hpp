#ifndef __KIT_STL_ANY_HPP__
#define __KIT_STL_ANY_HPP__

#include <new>

namespace kit {

class Any {
  template <typename T>
  friend T& any_cast(Any&);

 public:
  Any() : data_(nullptr) {}

  template <typename T>
  Any(const T& data) : data_(new Data<T>(data)) {}

  Any(const Any& other) : data_(other.data_->clone()) {}
  Any& operator=(const Any& other) {
    if (data_) {
      delete data_;
    }
    data_ = other.data_->clone();
    return *this;
  }
  Any(Any&& other) : data_(other.data_->clone()) {}
  Any& operator=(Any&& other) {
    if (data_) {
      delete data_;
    }
    data_ = other.data_->clone();
    return *this;
  }

 private:
  struct Base {
    virtual Base* clone() = 0;
    virtual ~Base() {}
  };

  template <typename T>
  struct Data : Base {
    Data(const T& data) : data_(data) {}
    Base* clone() override { return new Data<T>(data_); }

    T data_;
  };

  Base* data_;
};  // Any

template <typename T>
T& any_cast(Any& data) {
  return static_cast<Any::Data<T>*>(data.data_)->data_;
}

}  // namespace kit

#endif