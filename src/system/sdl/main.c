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

//#1346
static const char* getAppFolder()
{
	static char appFolder[TICNAME_MAX];

	char* path = SDL_GetPrefPath(TIC_PACKAGE, TIC_NAME);
	strcpy(appFolder, path);
	SDL_free(path);

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
				createMouseCursor();

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
