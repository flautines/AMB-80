// MIT License

// Copyright (c) 2017 Vadim Grigoruk @nesbox // grigoruk@gmail.com

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "tic.h"
#include <stddef.h>

#define POKE_N(P,I,V,A,B,C,D) do      \
{                                     \
  u8* val = (u8*)(P) + ((I) >> (A));  \
  u8 offset = ((I) & (B)) << (C);     \
  *val &= ~((D) << offset);           \
  *val |= ((V) & (D)) << offset;      \
} while(0)

#define PEEK_N(P,I,A,B,C,D) ( ( ((u8*)(P))[((I) >> (A))] >> ( ((I) & (B)) << (C) ) ) & (D) )

inline void tic_tool_poke4(void* addr, u32 index, u8 value)
{
  POKE_N(addr, index, value, 1,1,2,15);
}

inline u8 tic_tool_peek4(const void* addr, u32 index)
{
  return PEEK_N(addr, index, 1,1,2,15);
}

inline void tic_tool_poke2(void* addr, u32 index, u8 value)
{
  POKE_N(addr, index, value, 2,3,1,3);
}

inline u8 tic_tool_peek2(const void* addr, u32 index)
{
    return PEEK_N(addr, index, 2,3,1,3);
}

inline void tic_tool_poke1(void* addr, u32 index, u8 value)
{
  POKE_N(addr, index, value, 3,7,0,1);
}

inline u8 tic_tool_peek1(const void* addr, u32 index)
{
  return PEEK_N(addr, index, 3,7,0,1);
}

#undef PEEK_N
#undef POKE_N
