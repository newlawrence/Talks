#ifndef FOO_TABLE_HPP
#define FOO_TABLE_HPP

#include <memory_resource>
#include <memory>


namespace foo {

using allocator_t = std::pmr::polymorphic_allocator<std::byte>;


struct FooConcept {
    void(*copy)(void const* other, void*& self, allocator_t alloc);
    void(*transfer)(void* other, void*& self, allocator_t alloc);
    void(*move)(void*& other, void*& self, allocator_t alloc);
    void(*destroy)(void* self, allocator_t alloc);
    void(*bar)(void const* self);
};


template<typename T, std::size_t n>
FooConcept const FooDispatcherSBO{
    [](void const* other, void*& self, allocator_t alloc) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        auto size = n;
        std::align(alignof(T), sizeof(T), self, size);
        allocator.construct(
            reinterpret_cast<T*>(self),
            *reinterpret_cast<T const*>(other)
        );
    },
    [](void* other, void*& self, allocator_t alloc) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        auto size = n;
        std::align(alignof(T), sizeof(T), self, size);
        allocator.construct(
            reinterpret_cast<T*>(self),
            std::move(*reinterpret_cast<T*>(other))
        );
    },
    [](void*& other, void*& self, allocator_t alloc) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        auto size = n;
        std::align(alignof(T), sizeof(T), self, size);
        allocator.construct(
            reinterpret_cast<T*>(self),
            std::move(*reinterpret_cast<T*>(other))
        );
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
        self = allocator.allocate(1);
        allocator.construct(
            reinterpret_cast<T*>(self),
            *reinterpret_cast<T const*>(other)
        );
    },
    [](void* other, void*& self, allocator_t alloc) {
        auto allocator = std::pmr::polymorphic_allocator<T>{ alloc };
        self = allocator.allocate(1);
        allocator.construct(
            reinterpret_cast<T*>(self),
            std::move(*reinterpret_cast<T*>(other))
        );
    },
    [](void*& other, void*& self, [[maybe_unused]] allocator_t alloc) {
        self = other;
        other = nullptr;
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
