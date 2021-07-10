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

#include <assert.h>

static_assert(sizeof(tic80_input) == 12, "tic80_input");

static bool isKeyPressed(const tic80_keyboard* input, tic_key key)
{
    for (s32 i = 0; i < TIC80_KEY_BUFFER; i++)
        if (input->keys[i] == key)
            return true;
    
    return false;
}

void tic_core_tick_io(tic_mem* memory)
{
    tic_core* core = (tic_core*)memory;

    // process gamepad
    for (s32 i = 0; i < COUNT_OF(core->state.gamepads.holds); i++)
    {
        u32 mask = 1 << i;
        u32 prevDown = core->state.gamepads.previous.data & mask;
        u32 down = memory->ram.input.gamepads.data & mask;

        u32* hold = &core->state.gamepads.holds[i];
        if (prevDown && prevDown == down) (*hold)++;
        else *hold = 0;
    }

    // process keyboard
    for (s32 i = 0; i < tic_keys_count; i++)
    {
        bool prevDown = isKeyPressed(&core->state.keyboard.previous, i);
        bool down = isKeyPressed(&memory->ram.input.keyboard, i);

        u32* hold = &core->state.keyboard.holds[i];

        if (prevDown && down) (*hold)++;
        else *hold = 0;
    }
}
