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

#define EARLY_CLIP(x, y, width, height) \
  ( \
    (((y)+(height)-1) < core->state.clip.t) \
    || (((x)+(width)-1) < core->state.clip.l) \
    || ((y) >= core->state.clip.b) \
    || ((x) >= core->state.clip.r) \
  )

//#271
static s32 drawChar(tic_core* core, tic_tileptr* font_char, s32 x, s32 y, s32 scale, bool fixed, u8* mapping)
{
  enum { Size = TIC_SPRITESIZE };

  s32 j = 0, start = 0, end = Size;

  if (!fixed) 
  {
    for (s32 i = 0; i < Size; i++)
    {
      for (j = 0; j < Size; j++)
        if (mapping[tic_tilesheet_gettilepix(font_char, i, j)] != TRANSPARENT_COLOR) break;
      if ( j < Size) break; else start++;
    }
    for (s32 i = Size - 1; i >= start; i--)
    {
      for ( j = 0; j < Size; j++)
        if (mapping[tic_tilesheet_gettilepix(font_char, i, j)] != TRANSPARENT_COLOR) break;
      if (j < Size) break; else end--;
    }
  }
  s32 width = end - start;

  if (EARLY_CLIP(x, y, Size * scale, Size * scale)) return width;

  s32 colStart = start, colStep = 1, rowStart = 0, rowStep = 1;

  for (s32 i = 0, col = colStart, xs = x; i < width; i++, col += colStep, xs += scale)
  {
    for (s32 j = 0, row = rowStart, ys = y; j < Size; j++, row += rowStep, ys += scale)
    {
      u8 color = tic_tilesheet_gettilepix(font_char, col, row);
      if (mapping[color] != TRANSPARENT_COLOR)
        drawRect(core, xs, ys, scale, scale, mapping[color]);
    }
  }

  return width;
}

//#307
static s32 drawText(tic_core* core, tic_tilesheet* font_face, const char* text, s32 x, s32 y, s32 width, s32 height, bool fixed, u8* mapping, s32 scale, bool alt)
{
  s32 pos = x;
  s32 MAX = x;
  char sym = 0;

  while ((sym = *text++))
  {
    if (sym == '\n')
    {
      if (pos > MAX)
        MAX = pos;

      pos = x;
      y += height * scale;
    }
    else 
    {
      tic_tileptr font_char =tic_tilesheet_gettile(font_face, alt * TIC_FONT_CHARS / 2 + sym, true);
      s32 size = drawChar(core, &font_char, pos, y, scale, fixed, mapping);
      pos += ((!fixed && size) ? size + 1 : width) * scale;
    }
  }

  return pos > MAX ? pos - x : MAX - x;
}

//#333
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

//#385
s32 tic_api_print(tic_mem* memory, const char* text, s32 x, s32 y, u8 color, bool fixed, s32 scale, bool alt)
{
  u8 mapping[] = { 255, color };
  tic_tilesheet font_face = getTileSheetFromSegment(memory, 1);

  u8 width = alt ? TIC_ALTFONT_WIDTH : TIC_FONT_WIDTH;
  if (!fixed) width -= 2;
  return drawText((tic_core*)memory, &font_face, text, x, y, width, TIC_FONT_HEIGHT, fixed, mapping, scale, alt);
}

