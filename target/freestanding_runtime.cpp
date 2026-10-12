#include <cstddef>
#include <cstdint>

// Tiny target-only runtime support. ETCH does not provide malloc/new and the
// application code must remain allocation-free. These helpers only satisfy
// compiler-generated freestanding operations.
extern "C" void* memcpy(void* dst,const void* src,std::size_t n) noexcept {
 auto* d=static_cast<std::uint8_t*>(dst);auto* s=static_cast<const std::uint8_t*>(src);
 for(std::size_t i=0;i<n;++i)d[i]=s[i];return dst;
}
extern "C" void* memset(void* dst,int v,std::size_t n) noexcept {
 auto* d=static_cast<std::uint8_t*>(dst);for(std::size_t i=0;i<n;++i)d[i]=static_cast<std::uint8_t>(v);return dst;
}
extern "C" void* memmove(void* dst,const void* src,std::size_t n) noexcept {
 auto* d=static_cast<std::uint8_t*>(dst);auto* s=static_cast<const std::uint8_t*>(src);
 if(d<s){for(std::size_t i=0;i<n;++i)d[i]=s[i];}
 else if(d>s){for(std::size_t i=n;i>0;--i)d[i-1]=s[i-1];}
 return dst;
}
extern "C" void __cxa_pure_virtual() noexcept { for(;;){} }

// A virtual deleting-destructor thunk may reference operator delete even though
// ETCH never dynamically allocates R2 platform objects. Trap if it is ever used.
void operator delete(void*) noexcept { for(;;){} }
void operator delete(void*,std::size_t) noexcept { for(;;){} }
