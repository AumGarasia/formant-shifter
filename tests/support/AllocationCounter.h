#pragma once

#include <cstddef>

namespace testsupport
{

namespace detail
{
    extern thread_local bool counting;
    extern thread_local std::size_t allocations;
    extern thread_local std::size_t firstAllocationSize;

/** 
    Called for every allocation made while a scope is alive.
    Set a debugger breakpoint here to see who is allocating. 
*/
   void reportAllocation (std::size_t size);
}   // namespace detail

/**
    While alive, counts every operator-new allocation made by the calling thread.
    Do NOT use Catch2 assertions or build strings while a scope is alive.
*/
class AllocationScope
{
public:
    AllocationScope()
    {
        detail::allocations = 0;
        detail::firstAllocationSize = 0;
        detail::counting = true;
    }

    ~AllocationScope() { detail::counting = false; }

    AllocationScope (const AllocationScope&) = delete;
    AllocationScope& operator= (const AllocationScope&) = delete;

    std::size_t count() const { return detail::allocations; }
    std::size_t firstSize() const { return detail::firstAllocationSize; }
};

}   // namespace testsupport