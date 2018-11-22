#include "foo_wrapper.hpp"

using namespace foo;


FooWrapper::FooWrapper(FooWrapper const& other, allocator_type alloc)
    : allocator_{ alloc }
    , dispatcher_{ other.dispatcher_ }
{
    other.dispatcher_->copy(
        &const_cast<Storage&>(other.storage_),
        &storage_,
        &allocator_
    );
}

FooWrapper::FooWrapper(FooWrapper const& other)
    : FooWrapper{ other, other.allocator_ }
{}

FooWrapper::FooWrapper(FooWrapper&& other, allocator_type alloc)
    : allocator_{ alloc }
    , dispatcher_{ other.dispatcher_ }
{
    if (allocator_ == other.allocator_)
        other.dispatcher_->move(&other.storage_, &storage_, &allocator_);
    else  // cannot free memory later if allocators are incompatible
        other.dispatcher_->copy(&other.storage_, &storage_, &allocator_);
}

FooWrapper::FooWrapper(FooWrapper&& other) noexcept  // guaranteed
    : FooWrapper{ std::move(other), other.allocator_ }
{}  // if SBO mandated noexcept move (otherwise PMR), if PMR swap of pointers

FooWrapper::~FooWrapper() {
    dispatcher_->destroy(&storage_, &allocator_);
}

FooWrapper& FooWrapper::operator=(FooWrapper const& other) {
    dispatcher_->destroy(&storage_, &allocator_);
    dispatcher_ = other.dispatcher_;  // never forget reassigning dispatchers
    other.dispatcher_->copy(
        &const_cast<Storage&>(other.storage_),
        &storage_,
        &allocator_
    );
    return *this;
}

FooWrapper& FooWrapper::operator=(FooWrapper&& other) {
    dispatcher_->destroy(&storage_, &allocator_);
    dispatcher_ = other.dispatcher_;  // never forget reassigning dispatchers
    if (allocator_ == other.allocator_)
        other.dispatcher_->move(&other.storage_, &storage_, &allocator_);
    else  // cannot free memory later if allocators are incompatible
        other.dispatcher_->copy(&other.storage_, &storage_, &allocator_);
    return *this;
}

void FooWrapper::bar() const {
    dispatcher_->bar(&const_cast<Storage&>(storage_));
}
