#ifndef FOO_WRAPPER_HPP
#define FOO_WRAPPER_HPP

#include "foo_table.hpp"

#include <type_traits>

#include <memory_resource>
#include <memory>


namespace foo {

class FooWrapper {
public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

private:
    static std::size_t constexpr BUFFER_SIZE = 16;

    allocator_type allocator_;

    FooConcept const* dispatcher_;
    union Storage {
        std::aligned_storage_t<BUFFER_SIZE> local;
        void* remote;
    } storage_;

public:
    template<
        typename T,
        typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<T>, FooWrapper> &&
            !std::is_base_of_v<FooWrapper, std::decay_t<T>>
        >
    >
    explicit FooWrapper(T&& obj, allocator_type alloc={})
        : allocator_{ alloc }
    {
        std::pmr::polymorphic_allocator<T> allocator{ alloc };
        T* storage;

        if constexpr (
            sizeof(T) <= BUFFER_SIZE &&
            std::is_nothrow_move_constructible_v<T>
        ) {
            dispatcher_ = &FooDispatcherSBO<T, Storage>;
            storage = reinterpret_cast<T*>(&storage_.local);
        }
        else {
            dispatcher_ = &FooDispatcherPMR<T, Storage>;
            storage_.remote = allocator.allocate(1);
            storage = reinterpret_cast<T*>(storage_.remote);
        }
        allocator.construct(storage, std::forward<T>(obj));
    }

    FooWrapper(FooWrapper const& other, allocator_type alloc);
    FooWrapper(FooWrapper const& other);

    FooWrapper(FooWrapper&& other, allocator_type alloc);
    FooWrapper(FooWrapper&& other) noexcept;

    ~FooWrapper();

    FooWrapper& operator=(FooWrapper const& other);
    FooWrapper& operator=(FooWrapper&& other);

    void bar() const;
};

}

#endif
