// MIT License

// Copyright (c) 2020 Vadim Grigoruk @nesbox // grigoruk@gmail.com

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

#include "api.h"
#include "core.h"
#include "tilesheet.h"

#include <string.h>
#include <stdlib.h>

#define TRANSPARENT_COLOR 255

typedef void(*PixelFunc)(tic_mem* memory, s32 x, s32 y, u8 color);

//#34
static tic_tilesheet getTileSheetFromSegment(tic_mem* memory, u8 segment)
{
  u8* src;
  switch (segment)
  {
    case 0:
    case 1:
      src = (u8*)&memory->ram.font.data;
      break;
    default:
      src = (u8*)&memory->ram.tiles.data;
      break;      
  }

  return tic_tilesheet_get(segment, src);
}

//#385
s32 tic_api_print(tic_mem* memory, const char* text, s32 x, s32 y, u8 color, bool fixed, s32 scale, bool alt)
{
  u8 mapping[] = { 255, color };
  tic_tilesheet font_face = getTileSheetFromSegment(memory, 1);

  u8 width = alt ? TIC_ALTFONT_WIDTH : TIC_FONT_WIDTH;
  if (!fixed) width -= 2;
  return drawText((tic_core*)memory, &font_face, text, x, y, width, TIC_FONT_HEIGHT, fixed, mapping, scale, alt);
}
void tic_api_clip(tic_mem* memory, s32 x, s32 y, s32 width, s32 height)
{
  tic_core* core = (tic_core*)memory;

  core->state.clip.l = x;
  core->state.clip.t = y;
  core->state.clip.r = x + width;
  core->state.clip.b = y + height;

  if (core->state.clip.l < 0) core->state.clip.l = 0;
  if (core->state.clip.t < 0) core->state.clip.t = 0;
  if (core->state.clip.r > TIC80_WIDTH) core->state.clip.r = TIC80_WIDTH;
  if (core->state.clip.b > TIC80_HEIGHT) core->state.clip.b = TIC80_HEIGHT;
}
