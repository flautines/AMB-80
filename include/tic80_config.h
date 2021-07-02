// MIT License

// Copyright (c) 2017 Vadim Grigoruk @nesbox

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

#if !defined(TIC_BUILD_WITH_LUA)    && \
    !defined(TIC_BUILD_WITH_MOON)   && \
    !defined(TIC_BUILD_WITH_FENNEL) && \
    !defined(TIC_BUILD_WITH_JS)     && \
    !defined(TIC_BUILD_WITH_WREN)   && \
    !defined(TIC_BUILD_WITH_SQUIRREL)

#   define TIC_BUILD_WITH_LUA      1
#   define TIC_BUILD_WITH_MOON     1
#   define TIC_BUILD_WITH_FENNEL   1
#   define TIC_BUILD_WITH_JS       1
#   define TIC_BUILD_WITH_WREN     1
#   define TIC_BUILD_WITH_SQUIRREL 1

#endif

#define TIC_BUILD_WITH_LUA  1

#if defined(TIC_BUILD_WITH_LUA)
#   define TIC_LUA_DEF(macro) macro(lua, ".lua", "--", lua_State)
#endif

#define SCRIPT_LIST(macro) \
    TIC_LUA_DEF (macro)

#if defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#   undef __TIC_WINDOWS__
#   define __TIC_WINDOWS__ 1
#endif

#if defined(linux) || defined(__linux) || defined(__linux__)
#   undef __TIC_LINUX
#   define __TIC_LINUX__ 1
#endif

#ifndef TIC80_API
#   define TIC80_API
#endif
