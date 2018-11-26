#ifndef FOO_WRAPPER_HPP
#define FOO_WRAPPER_HPP

#include "foo_table.hpp"

#include <type_traits>


namespace foo {

class FooWrapper {
public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

private:
    static std::size_t constexpr BUFFER_SIZE = 3 * sizeof(void*);

    allocator_type allocator_;
    FooConcept const* dispatcher_;
    void* self_;
    std::aligned_storage_t<BUFFER_SIZE, alignof(void*)> buffer_;

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
        , dispatcher_{ &FooDispatcherPMR<T> }
        , self_{ &buffer_ }
    {
        std::pmr::polymorphic_allocator<T> allocator{ alloc };

        if constexpr (
            std::is_nothrow_move_constructible_v<T> &&
            sizeof(T) <= BUFFER_SIZE
        ) {
            auto size = BUFFER_SIZE;
            if (std::align(alignof(T), sizeof(T), self_, size))
                dispatcher_ = &FooDispatcherSBO<T, BUFFER_SIZE>;
            else
                self_ = allocator.allocate(1);
        }
        else
            self_ = allocator.allocate(1);
        allocator.construct(reinterpret_cast<T*>(self_), std::forward<T>(obj));
    }

    FooWrapper(FooWrapper const& other, allocator_type alloc);
    FooWrapper(FooWrapper const& other);

    FooWrapper(FooWrapper&& other, allocator_type alloc);
    FooWrapper(FooWrapper&& other) noexcept;

    ~FooWrapper();

    FooWrapper& operator=(FooWrapper const& other);
    FooWrapper& operator=(FooWrapper&& other);

    allocator_type get_allocator() const;

    void bar() const;
};

}

#endif
