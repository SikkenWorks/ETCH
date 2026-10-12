#pragma once
#include <array>
#include <cstddef>
#include <utility>

namespace etch {
template<class T,std::size_t Capacity>
class FixedQueue {
 static_assert(Capacity>0,"FixedQueue capacity must be non-zero");
public:
 constexpr std::size_t capacity() const noexcept {return Capacity;}
 std::size_t size() const noexcept {return size_;}
 std::size_t dropped() const noexcept {return dropped_;}
 bool empty() const noexcept {return size_==0;}
 bool full() const noexcept {return size_==Capacity;}
 bool push(const T& value) noexcept {
  if(full()){++dropped_;return false;}
  data_[size_++]=value;return true;
 }
 bool push(T&& value) noexcept {
  if(full()){++dropped_;return false;}
  data_[size_++]=std::move(value);return true;
 }
 void clear() noexcept {size_=0;}
 template<class Predicate,class Consumer>
 std::size_t drainIf(Predicate predicate,Consumer consume) noexcept {
  std::size_t write=0,drained=0;
  for(std::size_t read=0;read<size_;++read){
   if(predicate(data_[read])){consume(data_[read]);++drained;}
   else{if(write!=read)data_[write]=std::move(data_[read]);++write;}
  }
  size_=write;return drained;
 }
 const T& operator[](std::size_t index) const noexcept {return data_[index];}
private:
 std::array<T,Capacity> data_{};
 std::size_t size_=0,dropped_=0;
};
}
