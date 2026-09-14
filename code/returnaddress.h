/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

#pragma once

#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#define OPENTS_RETURN_ADDRESS() reinterpret_cast<std::uintptr_t>(_ReturnAddress())
#elif defined(__clang__) || defined(__GNUC__)
#define OPENTS_RETURN_ADDRESS() reinterpret_cast<std::uintptr_t>(__builtin_return_address(0))
#else
#define OPENTS_RETURN_ADDRESS() std::uintptr_t{0}
#endif
