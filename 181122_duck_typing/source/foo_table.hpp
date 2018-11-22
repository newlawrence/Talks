#ifndef FOO_TABLE_HPP
#define FOO_TABLE_HPP


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

template<typename T, typename U>
decltype(auto) local_cast(void* self) {  // local is a value
    return reinterpret_cast<T>(&static_cast<U>(self)->local);
}

template<typename T, typename U>
decltype(auto) remote_cast(void* self) {  // remote is a pointer
    if constexpr (std::is_pointer_v<std::remove_pointer_t<T>>)
        return reinterpret_cast<T>(&static_cast<U>(self)->remote);
    else
        return reinterpret_cast<T>(static_cast<U>(self)->remote);
}

}


// Foo dispatcher interface

struct FooConcept {
    void(*bar)(void* self);

    void(*copy)(void* other, void* self, void* alloc);
    void(*move)(void* other, void* self, void* alloc);
    void(*destroy)(void* self, void* alloc);
};


// This dispatcher makes use of local storage, moves shall be performed only
// on non-throwing objects to ensure noexcept move constructor of the wrapper

template<typename T, typename U>    // U == union FooWrapper::Storage;
FooConcept const FooDispatcherSBO{
    [](void* self) { detail::local_cast<T*, U*>(self)->bar(); },  // bar

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
    }
};

// This dispatcher makes use of polymorphic allocators,  moves shall be
// performed only when the allocators are interchangeable

template<typename T, typename U>  // U == union FooWrapper::Storage;
FooConcept const FooDispatcherPMR{
    [](void* self) { detail::remote_cast<T*, U*>(self)->bar(); },  // bar

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
        data = nullptr;  // Ensures safe destruction on moved from objects
    },
    [](void* self, void* alloc) {               // destroy
        auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::remote_cast<T*, U*>(self);
        if (storage)  // Avoid calling destructor on a null location
            std::destroy_at(storage);  // or 'allocator.destroy(storage);'
        allocator.deallocate(storage, 1);
    }
};

}

#endif
