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

#include "studio/system.h"
#include "tools.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <SDL.h>
#define STUDIO_PIXEL_FORMAT SDL_PIXELFORMAT_ABGR8888

//#66
enum
{
	tic_key_board = tic_keys_count + 1,
	tic_touch_size,
};

typedef enum
{
	HandCursor,
	IBeamCursor,
	ArrowCursor
} CursorType;

//#79
static const SDL_SystemCursor SystemCursors[] = 
{
	[HandCursor] = SDL_SYSTEM_CURSOR_HAND,
	[IBeamCursor] = SDL_SYSTEM_CURSOR_IBEAM,
	[ArrowCursor] = SDL_SYSTEM_CURSOR_ARROW
};

//#86
#define Renderer SDL_Renderer
#define Texture SDL_Texture
#define Rect SDL_Rect
#define destroyTexture SDL_DestroyTexture
#define destroyRenderer(render) SDL_DestroyRenderer(render)
#define renderPresent SDL_RenderPresent
#define renderClear SDL_RenderClear
#define renderCopy(render, tex, src, dst) SDL_RenderCopy(render, tex, &src, &dst)

//#106
static struct
{
	Studio* studio;

	SDL_Window* window;

	struct
	{
		Renderer* renderer;
		Texture* texture;
	} gpu;

	struct
	{
		SDL_Joystick* ports[TIC_GAMEPADS];
		tic80_gamepads joystick;

	} gamepad;
	
	struct
	{
		bool state[tic_keys_count];
		char text;

	} keyboard;

	struct
	{
		Texture* texture;
		const u8* src;
		SDL_Cursor* cursors[COUNT_OF(SystemCursors)];
	} mouse;

	struct
	{
		SDL_AudioSpec		spec;
		SDL_AudioDeviceID	device;
	} audio;

} platform;

//#212
static void initSound()
{
	SDL_AudioSpec want = 
	{
		.freq = TIC80_SAMPLERATE,
		.format = AUDIO_S16,
		.channels = TIC_STEREO_CHANNELS,
		.userdata = NULL,
	};

	platform.audio.device = SDL_OpenAudioDevice(NULL, 0, &want, &platform.audio.spec, 0);

}

//#236
static void setWindowIcon()
{
	enum{ Size = 64, TileSize = 16, ColorKey = 14, Cols = TileSize / TIC_SPRITESIZE, Scale = Size/TileSize};

	DEFER(u32* pixels = SDL_malloc(Size * Size * sizeof(u32)), SDL_free(pixels))
	{
		const u32* pal = tic_tool_palette_blit(&platform.studio->config()->cart->bank0.palette.scn, platform.studio->tic->screen_format);

		for (s32 j = 0, index = 0; j < Size; j++)
			for (s32 i = 0; i < Size; i++, index++)
			{
				u8 color = getSpritePixel(platform.studio->config()->cart->bank0.tiles.data, i/Scale, j/Scale);
				pixels[index] = color == ColorKey ? 0 : pal[color];
			}
		
		DEFER(SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, Size, Size,
			sizeof(s32) * BITS_IN_BYTE, Size * sizeof(s32),
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000), SDL_FreeSurface(surface))
		{
			SDL_SetWindowIcon(platform.window, surface);
		}
	}
}

//#418
static void initGPU()
{
	platform.gpu.renderer = SDL_CreateRenderer(platform.window, -1, SDL_RENDERER_ACCELERATED);
	platform.gpu.texture = SDL_CreateTexture(platform.gpu.renderer, STUDIO_PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING, TIC80_FULLWIDTH, TIC80_FULLHEIGHT);
}

//#1346
static const char* getAppFolder()
{
	static char appFolder[TICNAME_MAX];

	char* path = SDL_GetPrefPath(TIC_PACKAGE, TIC_NAME);
	strcpy(appFolder, path);
	SDL_free(path);

}

//#1402
void tic_sys_fullscreen()
{
	SDL_SetWindowFullscreen(platform.window,
		SDL_GetWindowFlags(platform.window) & SDL_WINDOW_FULLSCREEN_DESKTOP ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

//#1522
static void gpuTick()
{
	tic_mem* tic = platform.studio->tic;

	tic_sys_poll();

	if (platform.studio->quit)
	{
		return;
	}

	platform.studio->tick();
	renderClear(platform.gpu.renderer);
	updateTextureBytes(platform.gpu.texture, tic->screen, TIC80_FULLWIDTH, TIC80_FULLHEIGHT);
}

//#1642
static void createMouseCursors()
{
	for (s32 i = 0; i < COUNT_OF(platform.mouse.cursors); i++)
		platform.mouse.cursors[i] = SDL_CreateSystemCursor(SystemCursors[i]);
}

//#1648
static s32 start(s32 argc, char **argv, const char* folder)
{
	platform.studio = studioInit(argc, argv, TIC80_SAMPLERATE, folder);

	SCOPE(platform.studio->close())
	{
		if (platform.studio->config()->cli)
		{
			while (!platform.studio->quit)
				platform.studio->tick();
		}
		else
		{
			SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

			initSound();

			{
				const s32 Width = TIC80_FULLWIDTH * platform.studio->config()->uiScale;
				const s32 Height = TIC80_FULLHEIGHT * platform.studio->config()->uiScale;

				platform.window = SDL_CreateWindow(TIC_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					Width, Height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

				setWindowIcon();
				createMouseCursors();

				initGPU();

				if (platform.studio->config()->goFullscreen)
					tic_sys_fullscreen();
			}

			SDL_PauseAudioDevice(platform.audio.device, 0);

			{
				u64 nextTick = SDL_GetPerformanceCounter();
				const u64 Delta = SDL_GetPerformanceFrequency() / TIC80_FRAMERATE;

				while (!platform.studio->quit)
				{
					nextTick += Delta;

					gpuTick();

					{
						s64 delay = nextTick - SDL_GetPerformanceCounter();

						if (delay < 0)
							nextTick -= delay;
						else
							SDL_Delay((u32)(delay * 1000 / SDL_GetPerformanceFrequency()));
					}
				}
			}

			{
				destroyGPU();

				SDL_DestroyWindow(platform.window);
				SDL_CloseAudioDevice(platform.audio.device);

				for (s32 i = 0; i < COUNT_OF(platform.mouse.cursors); i++)
					SDL_FreeCursor(platform.mouse.cursors[i]);
			}
		}
	}

	return 0;
}
//#1818
s32 main(s32 argc, char **argv)
{
	const char* folder = getAppFolder();

	return start(argc, argv, folder);
}
