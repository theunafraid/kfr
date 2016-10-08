/** @addtogroup types
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once

#include "kfr.h"
#include "types.hpp"

namespace kfr
{

constexpr size_t index_undefined = static_cast<size_t>(-1);

#ifdef CMT_COMPILER_CLANG
#define KFR_NATIVE_SIMD 1
#endif

#ifdef KFR_NATIVE_SIMD

template <typename T, size_t N>
using internal_simd_type = T __attribute__((ext_vector_type(N)));

template <typename T, size_t N>
using simd = identity<internal_simd_type<T, N>>;

template <typename T, size_t N, bool A>
using simd_storage = internal::struct_with_alignment<simd<T, N>, A>;

template <typename T, size_t N, size_t... indices>
CMT_INLINE simd<T, sizeof...(indices)> simd_shuffle(const simd<T, N>& x, const simd<T, N>& y,
                                                    csizes_t<indices...>)
{
    return __builtin_shufflevector(x, y,
                                   ((indices == index_undefined) ? -1 : static_cast<intptr_t>(indices))...);
}
template <typename T, size_t N, size_t... indices>
CMT_INLINE simd<T, sizeof...(indices)> simd_shuffle(const simd<T, N>& x, csizes_t<indices...>)
{
    return __builtin_shufflevector(x, x,
                                   ((indices == index_undefined) ? -1 : static_cast<intptr_t>(indices))...);
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
CMT_INLINE simd<T, N> simd_read(const T* src)
{
    return ptr_cast<simd_storage<T, N, A>>(src)->value;
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(!is_poweroftwo(N)), typename = void>
CMT_INLINE simd<T, N> simd_read(const T* src)
{
    constexpr size_t first        = prev_poweroftwo(N);
    constexpr size_t rest         = N - first;
    constexpr auto extend_indices = cconcat(csizeseq<rest>, csizeseq<first - rest, index_undefined, 0>);
    constexpr auto concat_indices = csizeseq<N>;
    return simd_shuffle<T, first>(simd_read<first, A>(src),
                                  simd_shuffle<T, rest>(simd_read<rest, false>(src + first), extend_indices),
                                  concat_indices);
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
CMT_INLINE void simd_write(T* dest, const simd<T, N>& value)
{
    ptr_cast<simd_storage<T, N, A>>(dest)->value = value;
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(!is_poweroftwo(N)), typename = void>
CMT_INLINE void simd_write(T* dest, const simd<T, N>& value)
{
    constexpr size_t first = prev_poweroftwo(N);
    constexpr size_t rest  = N - first;
    simd_write<A, first>(dest, simd_shuffle(value, csizeseq<first>));
    simd_write<false, rest>(dest + first, simd_shuffle(value, csizeseq<rest, first>));
}

#define KFR_SIMD_CAST(T, N, X) __builtin_convertvector(X, ::kfr::simd<T, N>)
#define KFR_SIMD_BITCAST(T, N, X) ((::kfr::simd<T, N>)(X))
#define KFR_SIMD_BROADCAST(T, N, X) ((::kfr::simd<T, N>)(X))
#define KFR_SIMD_SHUFFLE(X, Y, ...) __builtin_shufflevector(X, Y, __VA_ARGS__)

#endif

}
