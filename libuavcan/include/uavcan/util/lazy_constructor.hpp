/*
 * Copyright (C) 2014 Pavel Kirienko <pavel.kirienko@gmail.com>
 */

#pragma once

#include <cstdlib>
#include <uavcan/fatal_error.hpp>
#include <uavcan/impl_constants.hpp>

namespace uavcan
{

template <typename T>
class LazyConstructor
{
    unsigned char data_[sizeof(T)] __attribute__((aligned(16)));  // TODO: compiler-independent alignment
    T* ptr_;

    void ensureConstructed() const
    {
        if (!ptr_)
            handleFatalError("LazyConstructor<T> is not constructed");
    }

    void ensureNotConstructed() const
    {
        if (ptr_)
            handleFatalError("LazyConstructor<T> is already constructed");
    }

    template <typename U> struct ParamType { typedef const U& Type; };
    template <typename U> struct ParamType<U&> { typedef U& Type; };
#if __cplusplus > 201100
    template <typename U> struct ParamType<U&&> { typedef U&& Type; };
#endif

public:
    LazyConstructor()
    : ptr_(NULL)
    {
        std::fill(data_, data_ + sizeof(T), 0);
    }

    LazyConstructor(const LazyConstructor<T>& rhs)
    : ptr_(NULL)
    {
        std::fill(data_, data_ + sizeof(T), 0);
        if (rhs)
            construct<const T&>(*rhs);  // Invoke copy constructor
    }

    ~LazyConstructor() { destroy(); }

    LazyConstructor<T>& operator=(const LazyConstructor<T>& rhs)
    {
        destroy();
        if (rhs)
            construct<const T&>(*rhs);  // Invoke copy constructor
        return *this;
    }

    bool isConstructed() const { return ptr_ != NULL; }

    operator T*() const { return ptr_; }

    const T* operator->() const { ensureConstructed(); return ptr_; }
    T*       operator->()       { ensureConstructed(); return ptr_; }

    const T& operator*() const { ensureConstructed(); return *ptr_; }
    T&       operator*()       { ensureConstructed(); return *ptr_; }

    void destroy()
    {
        if (ptr_)
            ptr_->~T();
        ptr_ = NULL;
        std::fill(data_, data_ + sizeof(T), 0);
    }

    void construct()
    {
        ensureNotConstructed();
        ptr_ = new (static_cast<void*>(data_)) T();
    }

// MAX_ARGS = 6
// TEMPLATE = '''
//     template <%s>
//     void construct(%s)
//     {
//         ensureNotConstructed();
//         ptr_ = new (static_cast<void*>(data_)) T(%s);
//     }'''
// for nargs in range(1, MAX_ARGS + 1):
//     nums = [(x + 1) for x in range(nargs)]
//     l1 = ['typename P%d' % x for x in nums]
//     l2 = ['typename ParamType<P%d>::Type p%d' % (x, x) for x in nums]
//     l3 = ['p%d' % x for x in nums]
//     print(TEMPLATE % (', '.join(l1), ', '.join(l2), ', '.join(l3)))

    template <typename P1>
    void construct(typename ParamType<P1>::Type p1)
    {
        ensureNotConstructed();
        ptr_ = new (static_cast<void*>(data_)) T(p1);
    }

    template <typename P1, typename P2>
    void construct(typename ParamType<P1>::Type p1, typename ParamType<P2>::Type p2)
    {
        ensureNotConstructed();
        ptr_ = new (static_cast<void*>(data_)) T(p1, p2);
    }

    template <typename P1, typename P2, typename P3>
    void construct(typename ParamType<P1>::Type p1, typename ParamType<P2>::Type p2, typename ParamType<P3>::Type p3)
    {
        ensureNotConstructed();
        ptr_ = new (static_cast<void*>(data_)) T(p1, p2, p3);
    }

    template <typename P1, typename P2, typename P3, typename P4>
    void construct(typename ParamType<P1>::Type p1, typename ParamType<P2>::Type p2, typename ParamType<P3>::Type p3,
                   typename ParamType<P4>::Type p4)
    {
        ensureNotConstructed();
        ptr_ = new (static_cast<void*>(data_)) T(p1, p2, p3, p4);
    }

    template <typename P1, typename P2, typename P3, typename P4, typename P5>
    void construct(typename ParamType<P1>::Type p1, typename ParamType<P2>::Type p2, typename ParamType<P3>::Type p3,
                   typename ParamType<P4>::Type p4, typename ParamType<P5>::Type p5)
    {
        ensureNotConstructed();
        ptr_ = new (static_cast<void*>(data_)) T(p1, p2, p3, p4, p5);
    }

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
    void construct(typename ParamType<P1>::Type p1, typename ParamType<P2>::Type p2, typename ParamType<P3>::Type p3,
                   typename ParamType<P4>::Type p4, typename ParamType<P5>::Type p5, typename ParamType<P6>::Type p6)
    {
        ensureNotConstructed();
        ptr_ = new (static_cast<void*>(data_)) T(p1, p2, p3, p4, p5, p6);
    }
};

}