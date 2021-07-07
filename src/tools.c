#include "tools.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>

extern void tic_tool_poke4(void* addr, u32 index, u8 value);
extern u8 tic_tool_peek4(const void* addr, u32 index);
extern void tic_tool_poke2(void* addr, u32 index, u8 value);
extern u8 tic_tool_peek2(const void* addr, u32 index);
extern void tic_tool_poke1(void* addr, u32 index, u8 value);
extern u8 tic_tool_peek1(const void* addr, u32 index);
