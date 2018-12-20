#ifndef FOO_TABLE_HPP
#define FOO_TABLE_HPP

#include <memory_resource>
#include <memory>


namespace foo {

using allocator_t = std::pmr::polymorphic_allocator<std::byte>;


struct FooConcept {
    void(*copy)(void const* other, void*& self, allocator_t alloc);
    void(*move)(void*& other, void*& self, allocator_t alloc, bool swappable);
    void(*destroy)(void* self, allocator_t alloc);
    void(*bar)(void const* self);
};


template<typename T, std::size_t n>
FooConcept const FooDispatcherSBO{
    [](void const* other, void*& self, allocator_t alloc) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        auto const& data = *reinterpret_cast<T const*>(other);
        auto size = n;
        std::align(alignof(T), sizeof(T), self, size);
        allocator.construct(reinterpret_cast<T*>(self), data);
    },
    [](void*& other, void*& self, allocator_t alloc, bool) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        auto& data = *reinterpret_cast<T*>(other);
        auto size = n;
        std::align(alignof(T), sizeof(T), self, size);
        allocator.construct(reinterpret_cast<T*>(self), std::move(data));
    },
    [](void* self, allocator_t alloc) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        allocator.destroy(reinterpret_cast<T*>(self));
    },
    [](void const* self) { reinterpret_cast<T const*>(self)->bar(); }
};

template<typename T>
FooConcept const FooDispatcherPMR{
    [](void const* other, void*& self, allocator_t alloc) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        auto const& data = *reinterpret_cast<T const*>(other);
        self = allocator.allocate(1);
        try {
            allocator.construct(reinterpret_cast<T*>(self), data);
        }
        catch (...) {
            allocator.deallocate(reinterpret_cast<T*>(self), 1);
            throw;
        }
    },
    [](void*& other, void*& self, allocator_t alloc, bool swappable) {
        if (swappable) {
            self = other;
            other = nullptr;
            return;
        }
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        auto& data = *reinterpret_cast<T*>(other);
        self = allocator.allocate(1);
        try {
            allocator.construct(reinterpret_cast<T*>(self), std::move(data));
        }
        catch (...) {
            allocator.deallocate(reinterpret_cast<T*>(self), 1);
            throw;
        }
    },
    [](void* self, allocator_t alloc) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        if (self)
            allocator.destroy(reinterpret_cast<T*>(self));
        allocator.deallocate(reinterpret_cast<T*>(self), 1);
    },
    [](void const* self) { reinterpret_cast<T const*>(self)->bar(); }
};

}

#endif
