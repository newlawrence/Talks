#include "foo_wrapper.hpp"

using namespace foo;


FooWrapper::FooWrapper(FooWrapper const& other, allocator_type alloc)
    : allocator_{ alloc }
    , dispatcher_{ other.dispatcher_ }
{
    other.dispatcher_->copy(
        &const_cast<Data&>(other.self_),
        &self_,
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
        other.dispatcher_->move(&other.self_, &self_, &allocator_);
    else
        other.dispatcher_->copy(&other.self_, &self_, &allocator_);
}

FooWrapper::FooWrapper(FooWrapper&& other) noexcept
    : FooWrapper{ std::move(other), other.allocator_ }
{}

FooWrapper::~FooWrapper() { dispatcher_->destroy(&self_, &allocator_); }

FooWrapper& FooWrapper::operator=(FooWrapper const& other) {
    dispatcher_->destroy(&self_, &allocator_);
    dispatcher_ = other.dispatcher_;
    other.dispatcher_->copy(
        &const_cast<Data&>(other.self_),
        &self_,
        &allocator_
    );
    return *this;
}

FooWrapper& FooWrapper::operator=(FooWrapper&& other) {
    dispatcher_->destroy(&self_, &allocator_);
    dispatcher_ = other.dispatcher_;
    if (allocator_ == other.allocator_)
        other.dispatcher_->move(&other.self_, &self_, &allocator_);
    else
        other.dispatcher_->copy(&other.self_, &self_, &allocator_);
    return *this;
}

void FooWrapper::foo() const { dispatcher_->foo(&const_cast<Data&>(self_)); }
