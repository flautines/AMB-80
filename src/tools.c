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
extern s32 tic_tool_sfx_pos(s32 speed, s32 ticks);

//#110
u32* tic_tool_palette_blit(const tic_palette* srcpal, tic80_pixel_color_format fmt)
{
	static u32 pal[TIC_PALETTE_SIZE];

	const tic_rgb* src = srcpal->colors;
	const tic_rgb* end = src + TIC_PALETTE_SIZE;
	u8* dst = (u8*)pal;

	while (src != end)
	{
		switch (fmt) 
		{
		case TIC80_PIXEL_COLOR_BGRA8888:
			*dst++ = src->b;
			*dst++ = src->g;
			*dst++ = src->r;
			*dst++ = 0xff;
		break;
		case TIC80_PIXEL_COLOR_RGBA8888:
			*dst++ = src->r;
			*dst++ = src->g;
			*dst++ = src->b;
			*dst++ = 0xff;
		break;
		case TIC80_PIXEL_COLOR_ABGR8888:
			*dst++ = 0xff;
			*dst++ = src->b;
			*dst++ = src->g;
			*dst++ = src->r;			
		break;
		case TIC80_PIXEL_COLOR_ARGB8888:
			*dst++ = 0xff;		
			*dst++ = src->r;
			*dst++ = src->g;
			*dst++ = src->r;
		break;
		}
		src++;

	}

	return pal;
}

//#170
bool tic_tool_empty(const void* buffer, s32 size)
{
	for(const u8* ptr = buffer, *end = ptr + size; ptr < end;)
		if (*ptr++)
			return false;
	
	return true;
}

//#213
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

