//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace hpx {

#if defined(HPX_HAVE_CXX20_STD_CONSTRUCT_AT)
    using std::construct_at;
#else
    template <typename T, typename... Ts,
        typename Enable = std::void_t<decltype(
            ::new (std::declval<void*>()) T(std::declval<Ts>()...))>>
    constexpr T* construct_at(T* addr, Ts&&... ts) noexcept(noexcept(
        ::new (const_cast<void*>(static_cast<const volatile void*>(addr)))
            T(HPX_FORWARD(Ts, ts)...)))
    {
        return ::new (const_cast<void*>(
            static_cast<const volatile void*>(addr))) T(HPX_FORWARD(Ts, ts)...);
    }
#endif

#if defined(HPX_HAVE_P1144_STD_RELOCATE_AT)
    using std::relocate_at;
#else
    template <typename T>
    constexpr T* relocate_at(T* src, T* dst)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        T *ret = ::new ((void*)dst) T(HPX_MOVE(*src));
        dst->~T();
        return ret;
    }
#endif

}    // namespace hpx
