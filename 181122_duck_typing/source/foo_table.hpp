#ifndef FOO_TABLE_HPP
#define FOO_TABLE_HPP


namespace foo {

namespace detail {

template<typename T>
decltype(auto) rebind(void* alloc) {
    return std::pmr::polymorphic_allocator<T>{
        *static_cast<std::pmr::polymorphic_allocator<std::byte>*>(alloc)
    };
}


template<typename T, typename U>
decltype(auto) local_cast(void* self) {
    return reinterpret_cast<T>(&static_cast<U>(self)->local);
}

template<typename T, typename U>
decltype(auto) remote_cast(void* self) {
    if constexpr (std::is_pointer_v<std::remove_pointer_t<T>>)
        return reinterpret_cast<T>(&static_cast<U>(self)->remote);
    else
        return reinterpret_cast<T>(static_cast<U>(self)->remote);
}

}


struct FooConcept {
    void(*foo)(void* self);

    void(*copy)(void* other, void* self, void* alloc);
    void(*move)(void* other, void* self, void* alloc);
    void(*destroy)(void* self, void* alloc);
};


template<typename T, typename U>
FooConcept const FooDispatcherSBO{
    [](void* self) { detail::local_cast<T*, U*>(self)->foo(); },

    [](void* other, void* self, void* alloc) {
        auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::local_cast<T*, U*>(self);
        auto& data = *detail::local_cast<T*, U*>(other);
        allocator.construct(storage, data);
    },
    [](void* other, void* self, void* alloc) {
        auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::local_cast<T*, U*>(self);
        auto& data = *detail::local_cast<T*, U*>(other);
        allocator.construct(storage, std::move(data));
    },
    [](void* self, void* alloc) {
        [[maybe_unused]] auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::local_cast<T*, U*>(self);
        std::destroy_at(storage);
    }
};

template<typename T, typename U>
FooConcept const FooDispatcherPMR{
    [](void* self) { detail::remote_cast<T*, U*>(self)->foo(); },

    [](void* other, void* self, void* alloc) {
        auto allocator = detail::rebind<T>(alloc);
        auto& storage = *detail::remote_cast<T**, U*>(self);
        auto& data = *detail::remote_cast<T*, U*>(other);
        storage = allocator.allocate(1);
        allocator.construct(storage, data);
    },
    [](void* other, void* self, void* alloc) {
        [[maybe_unused]] auto allocator = detail::rebind<T>(alloc);
        auto& storage = *detail::remote_cast<T**, U*>(self);
        auto& data = *detail::remote_cast<T**, U*>(other);
        std::swap(storage, data);
        data = nullptr;
    },
    [](void* self, void* alloc) {
        auto allocator = detail::rebind<T>(alloc);
        auto storage = detail::remote_cast<T*, U*>(self);
        if (storage)
            std::destroy_at(storage);
        allocator.deallocate(storage, 1);
    }
};

}

#endif
