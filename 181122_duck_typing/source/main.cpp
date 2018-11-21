#include "foo_wrapper.hpp"

#include <iostream>
#include <array>

#include <cstring>


class SmallFoo {
public:
    void bar() const { std::cout << "I am a small foo!" << std::endl; }
};

class BigFoo {
    char str_[24];

public:
    BigFoo() noexcept { std::strcpy(str_, "I am a big foo!"); }
    BigFoo(BigFoo const& other) noexcept : BigFoo{} {}
    BigFoo(BigFoo&& other) noexcept : BigFoo{} {}

    void bar() const { std::cout << str_ << std::endl; }
};


int main() {
    std::array<std::byte, 256> buffer;
    std::pmr::monotonic_buffer_resource pool{ buffer.data(), buffer.size() };

    std::pmr::vector<foo::FooWrapper> foos{ &pool };
    foos.emplace_back(SmallFoo());
    foos.emplace_back(BigFoo());

    for (auto const& foo : foos)
        foo.bar();
    return 0;
}
