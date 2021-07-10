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

#include "cart.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef enum
{
    CHUNK_DUMMY,                    // 0
    CHUNK_TILES,                    // 1
    CHUNK_SPRITES,                  // 2
    CHUNK_COVER_DEP,                // 3 - deprecated chunk    
    CHUNK_MAP,                      // 4
    CHUNK_CODE,                     // 5
    CHUNK_FLAGS,                    // 6
    CHUNK_TEMP2,                    // 7
    CHUNK_TEMP3,                    // 8
    CHUNK_SAMPLES,                  // 9
    CHUNK_WAVEFORM,                 // 10
    CHUNK_TEMP4,                    // 11    
    CHUNK_PALETTE,                  // 12
    CHUNK_PATTERNS_DEP,             // 13 - deprecated chunk
    CHUNK_MUSIC,                    // 14
    CHUNK_PATTERNS,                 // 15
    CHUNK_CODE_ZIP,                 // 16
    CHUNK_DEFAULT,                  // 17
    CHUNK_SCREEN,                   // 18  
} ChunkType;

typedef struct
{
    u32 type:5; // ChunkType
    u32 bank:TIC_BANK_BITS;
    u32 size:TIC_BANKSIZE_BITS; // max chunk size is 64K
    u32 temp:8;
} Chunk;

static_assert(sizeof(Chunk) == 4, "tic_chunk_size");

static const u8 Sweetie16[] = {0x1a, 0x1c, 0x2c, 0x5d, 0x27, 0x5d, 0xb1, 0x3e, 0x53, 0xef, 0x7d, 0x57, 0xff, 0xcd, 0x75, 0xa7, 0xf0, 0x70, 0x38, 0xb7, 0x64, 0x25, 0x71, 0x79, 0x29, 0x36, 0x6f, 0x3b, 0x5d, 0xc9, 0x41, 0xa6, 0xf6, 0x73, 0xef, 0xf7, 0xf4, 0xf4, 0xf4, 0x94, 0xb0, 0xc2, 0x56, 0x6c, 0x86, 0x33, 0x3c, 0x57};
static const u8 Waveforms[] = {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};

static s32 chunkSize(const Chunk* chunk)
{
    return chunk->size == 0 && chunk->type == CHUNK_CODE ? TIC_BANK_SIZE : chunk->size;
}

void tic_cart_load(tic_cartridge* cart, const u8* buffer, s32 size)
{
    memset(cart, 0, sizeof(tic_cartridge));
    const u8* end = buffer + size;

#define LOAD_CHUNK(to) memcpy(&to, ptr, MIN(sizeof(to), chunk->size ? chunk->size : TIC_BANK_SIZE))

    // load palette chunk first
    {
        const u8* ptr = buffer;
        while (ptr < end)
        {
            const Chunk* chunk = (Chunk*)ptr;
            ptr += sizeof(Chunk);

            switch (chunk->type)
            {
            case CHUNK_PALETTE:
                LOAD_CHUNK(cart->banks[chunk->bank].palette);
                break;
            case CHUNK_DEFAULT:
                memcpy(&cart->banks[chunk->bank].palette, Sweetie16, sizeof Sweetie16);
                memcpy(&cart->banks[chunk->bank].sfx.waveforms, Waveforms, sizeof Waveforms);
                break;
            default: break;
            }

            ptr += chunkSize(chunk);
        }        
    }

    struct CodeChunk {s32 size; const char* data;} code[TIC_BANKS] = {0};

    {
        const u8* ptr = buffer;
        while (ptr < end)
        {
            const Chunk* chunk = (Chunk*)ptr;
            ptr += sizeof(Chunk);

            switch(chunk->type)
            {
            case CHUNK_TILES:       LOAD_CHUNK(cart->banks[chunk->bank].tiles);             break;
            case CHUNK_SPRITES:     LOAD_CHUNK(cart->banks[chunk->bank].sprites);           break;
            case CHUNK_MAP:         LOAD_CHUNK(cart->banks[chunk->bank].map);               break;
            case CHUNK_SAMPLES:     LOAD_CHUNK(cart->banks[chunk->bank].sfx.samples);       break;
            case CHUNK_WAVEFORM:    LOAD_CHUNK(cart->banks[chunk->bank].sfx.waveforms);     break;
            case CHUNK_MUSIC:       LOAD_CHUNK(cart->banks[chunk->bank].music.tracks);      break;
            case CHUNK_PATTERNS:    LOAD_CHUNK(cart->banks[chunk->bank].music.patterns);    break;
            case CHUNK_FLAGS:       LOAD_CHUNK(cart->banks[chunk->bank].flags);             break;
            case CHUNK_SCREEN:      LOAD_CHUNK(cart->banks[chunk->bank].screen);            break;
            case CHUNK_CODE:
                code[chunk->bank] = (struct CodeChunk){chunkSize(chunk), ptr};
                break;
            default: break;
            }

            ptr += chunkSize(chunk);
        }
#undef LOAD_CHUNK

        if (!*cart->code.data)
        {
            char* ptr = cart->code.data;
            RFOR(const struct CodeChunk*, chunk, code)
                if (chunk->data)
                {
                    memcpy(ptr, chunk->data, chunk->size);
                    ptr += chunk->size;
                }
        }
    }
}
