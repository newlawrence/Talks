#ifndef FOO_TABLE_HPP
#define FOO_TABLE_HPP

#include <memory>


namespace foo {

// Some auxiliary functions to save some boilerplate code related to castings

namespace detail {

template<typename T>
decltype(auto) rebind(void* alloc) {
    return std::pmr::polymorphic_allocator<T>{
        *static_cast<std::pmr::polymorphic_allocator<std::byte>*>(alloc)
    };
}


// The following functions are for internal use and only specialized on pointer
// types. Their purpose is to look similar to regular castings from pointer type
// U to pointer type T. Use: 'local_cast<T*, U*>(some_void_pointer)'

template<typename P, typename Q>
decltype(auto) local_cast(void* self) {  // local is a value
    using T = std::remove_pointer_t<P>;
    void* buffer = &static_cast<Q>(self)->local;
    auto size = sizeof(static_cast<Q>(self)->local);
    return reinterpret_cast<P>(std::align(alignof(T), sizeof(T), buffer, size));
}

template<typename P, typename Q>
decltype(auto) remote_cast(void* self) {  // remote is a pointer
    if constexpr (std::is_pointer_v<std::remove_pointer_t<P>>)
        return reinterpret_cast<P>(&static_cast<Q>(self)->remote);
    else
        return reinterpret_cast<P>(static_cast<Q>(self)->remote);
}

}


// Foo dispatcher interface

struct FooConcept {
    void(*copy)(void* other, void* self, void* alloc);
    void(*move)(void* other, void* self, void* alloc);
    void(*destroy)(void* self, void* alloc);
    void(*bar)(void* self);
    bool small;
};


// This dispatcher makes use of local storage, moves shall be performed only
// on non-throwing objects to ensure noexcept move constructor of the wrapper

template<typename T, typename U>    // U == union FooWrapper::Storage;
FooConcept const FooDispatcherSBO{
    [](void* other, void* self, void* alloc) {  // copy
        auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::local_cast<T*, U*>(self);
        auto& data = *detail::local_cast<T*, U*>(other);
        allocator.construct(storage, data);
    },
    [](void* other, void* self, void* alloc) {  // move
        auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::local_cast<T*, U*>(self);
        auto& data = *detail::local_cast<T*, U*>(other);
        allocator.construct(storage, std::move(data));
    },
    [](void* self, void* alloc) {               // destroy
        [[maybe_unused]] auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::local_cast<T*, U*>(self);
        std::destroy_at(storage);  // or 'allocator.destroy(storage);'
    },
    [](void* self) { detail::local_cast<T*, U*>(self)->bar(); },  // bar
    true  // small
};

// This dispatcher makes use of polymorphic allocators,  moves shall be
// performed only when the allocators are interchangeable

template<typename T, typename U>  // U == union FooWrapper::Storage;
FooConcept const FooDispatcherPMR{
    [](void* other, void* self, void* alloc) {  // copy
        auto allocator = detail::rebind<T>(alloc);
        auto& storage = *detail::remote_cast<T**, U*>(self);
        auto& data = *detail::remote_cast<T*, U*>(other);
        storage = allocator.allocate(1);
        allocator.construct(storage, data);
    },
    [](void* other, void* self, void* alloc) {  // move
        [[maybe_unused]] auto allocator = detail::rebind<T>(alloc);
        auto& storage = *detail::remote_cast<T**, U*>(self);
        auto& data = *detail::remote_cast<T**, U*>(other);
        storage = data;
        data = nullptr;  // ensures safe destruction on moved from objects
    },
    [](void* self, void* alloc) {               // destroy
        auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::remote_cast<T*, U*>(self);
        if (storage)  // avoid calling destructor on a null location
            std::destroy_at(storage);  // or 'allocator.destroy(storage);'
        allocator.deallocate(storage, 1);
    },
    [](void* self) { detail::remote_cast<T*, U*>(self)->bar(); },  // bar
    false  // small
};

}

#endif
