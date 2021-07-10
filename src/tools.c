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

const char* tic_tool_metatag(const char* code, const char* tag, const char* comment)
{
    const char* start = NULL;
    {
        static char format[] = "%s %s:";

        char* tagBuffer = malloc(strlen(format) + strlen(tag));

        if (tagBuffer)
        {
            sprintf(tagBuffer, format, comment, tag);
            if ((start = strstr(code, tagBuffer)))
                start += strlen(tagBuffer);
            free(tagBuffer);
        }
    }

    if (start)
    {
        const char* end = strstr(start, "\n");

        if (end)
        {
            while (*start <= ' ' && start < end) start++;
            while (*(end - 1) <= ' ' && end > start) end--;

            const s32 size = (s32)(end - start);

            char* value = (char*)malloc(size + 1);

            if (value)
            {
                memset(value, 0, size + 1);
                memcpy(value, start, size);

                return value;
            }
        }
    }

    return NULL;
}