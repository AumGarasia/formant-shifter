#include "AllocationCounter.h"

#include <algorithm>
#include <cstdlib>
#include <new>

namespace testsupport::detail
{

thread_local bool counting = false;
thread_local std::size_t allocations = 0;
thread_local std::size_t firstAllocationSize = 0;

__attribute__ ((noinline)) void reportAllocation (std::size_t size)
{
    if (allocations == 0)
        firstAllocationSize = size;

    ++allocations;
}

} // namespace testsupport::detail

namespace
{

void* allocate (std::size_t size)
{
    if (testsupport::detail::counting)
        testsupport::detail::reportAllocation (size);

    if (void* p = std::malloc (std::max<std::size_t> (size, 1)))
        return p;

    throw std::bad_alloc();
}

void* allocateAligned (std::size_t size, std::align_val_t alignment)
{
    if (testsupport::detail::counting)
        testsupport::detail::reportAllocation (size);

    const auto a = static_cast<std::size_t> (alignment);
    const auto rounded = (std::max<std::size_t> (size, 1) + a - 1) / a * a;   // aligned_alloc needs a multiple

    if (void* p = std::aligned_alloc (a, rounded))
        return p;

    throw std::bad_alloc();
}

} // namespace

// Replacement global allocation functions for the test executable.
void* operator new (std::size_t size)                              { return allocate (size); }
void* operator new[] (std::size_t size)                            { return allocate (size); }
void* operator new (std::size_t size, std::align_val_t a)          { return allocateAligned (size, a); }
void* operator new[] (std::size_t size, std::align_val_t a)        { return allocateAligned (size, a); }

void operator delete (void* p) noexcept                            { std::free (p); }
void operator delete[] (void* p) noexcept                          { std::free (p); }
void operator delete (void* p, std::size_t) noexcept               { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept             { std::free (p); }
void operator delete (void* p, std::align_val_t) noexcept          { std::free (p); }
void operator delete[] (void* p, std::align_val_t) noexcept        { std::free (p); }
void operator delete (void* p, std::size_t, std::align_val_t) noexcept   { std::free (p); }
void operator delete[] (void* p, std::size_t, std::align_val_t) noexcept { std::free (p); }