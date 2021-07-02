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

#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <tic80.h>

#define TIC80_WINDOW_SCALE 3
#define TIC80_WINDOW_TITLE "TIC-80"
#define TIC80_DEFAULT_CART "cart.tic"
#define TIC80_EXECUTABLE_NAME "player-sdl"

static struct {
    bool quit;
} state = { .quit = false, };

static void onExit()
{
    state.quit = true;
}

s32 runCart(void* cart, s32 size)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    SDL_Window* window = SDL_CreateWindow(TIC80_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, TIC80_FULLWIDTH * TIC80_WINDOW_SCALE, TIC80_FULLHEIGHT * TIC80_WINDOW_SCALE, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, TIC80_FULLWIDTH, TIC80_FULLHEIGHT);

    SDL_AudioDeviceID audioDevice = 0;
    SDL_AudioSpec audioSpec;
    SDL_AudioCVT cvt;
    bool audioStarted = false;
    s32 output = 0;

    {
        SDL_AudioSpec want = 
        {
            .freq = TIC80_SAMPLERATE,
            .format = AUDIO_S16,
            .channels = 2,
            .userdata = NULL,
        };

        audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &audioSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);

        SDL_BuildAudioCVT(&cvt, want.format, want.channels, audioSpec.freq, audioSpec.format, audioSpec.channels, audioSpec.freq);

        if (cvt.needed) {
            cvt.len = audioSpec.freq * audioSpec.channels * sizeof(s16) / TIC80_FRAMERATE;
            cvt.buf = SDL_malloc(cvt.len * cvt.len_mult);
        }
    }

    tic80_input input;
    SDL_memset(&input, 0, sizeof input);

    tic80* tic = tic80_create(audioSpec.freq);
    tic->callback.exit = onExit;
    tic80_load(tic, cart, size);

    if(!tic) {
        fprintf(stderr, "Failed to load cart data.");
        output = 1;
    }
}

s32 main(s32 argc, char** argv)
{
    const char* executable = argc > 0 ? argv[0] : TIC80_EXECUTABLE_NAME;
    const char* input = (argc > 1) ? argv[1] : TIC80_DEFAULT_CART;

    if (strcmp(input, "--help") == 0 || strcmp(input, "-h") == 0) {
        printf("Usage: %s <file>\n", executable);
        return 0;
    }

    FILE* file = fopen(input, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not load %s.\n\nUsage: %s <file>\n", input, argv[0]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    s32 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    void* cart = SDL_malloc(size);
    if (cart) fread(cart, size, 1, file);
    fclose(file);

    if (!cart) {
        fprintf(stderr, "Error reading %s.\n", input);
        return 1;
    }

    return runCart(cart, size);
}