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

#include "studio.h"

#include "editors/code.h"
#include "editors/sprite.h"
#include "editors/map.h"
#include "editors/world.h"
#include "editors/sfx.h"
#include "editors/music.h"
#include "screens/console.h"
#include "screens/surf.h"
#include "screens/dialog.h"
//#include "ext/history.h"
#include "net.h"
#include "wave_writer.h"
//#include "ext/gif.h"

//#include "ext/md5.h"
#include "screens/start.h"
#include "screens/run.h"
#include "screens/menu.h"
#include "config.h"
#include "cart.h"

#include "fs.h"

//#include "argparse.h"

#include <ctype.h>
#include <math.h>

#define TIC_EDITOR_BANKS (TIC_BANKS)

#define MD5_HASHSIZE 16
#define BG_ANIMATION_COLOR tic_color_dark_grey

#define FRAME_SIZE (TIC80_FULLWIDTH * TIC80_FULLHEIGHT * sizeof(u32))
#define POPUP_DUR (TIC80_FRAMERATE*2)

static const char VideoGif[] = "video%i.gif";
static const char ScreenGif[] = "screen%i.gif";

typedef struct
{
	u8 data[MD5_HASHSIZE];
} CartHash;

static const EditorMode Modes[] = 
{
	TIC_CODE_MODE,
	TIC_SPRITE_MODE,
	TIC_MAP_MODE,
	TIC_SFX_MODE,
	TIC_MUSIC_MODE,
};

static const EditorMode BankModes[] =
{
	TIC_SPRITE_MODE,
	TIC_MAP_MODE,
	TIC_SFX_MODE,
	TIC_MUSIC_MODE,
};

typedef struct
{
	bool down;
	bool click;

	tic_point start;
	tic_point end;

} MouseState;

//#107
static struct 
{
	Studio studio;

	tic80_local* tic80local;

	EditorMode mode;
	EditorMode prevMode;

	struct
	{
		MouseState state[3];
	};

	tic_key keycodes[KEYMAP_COUNT];

	EditorMode dialogMode;

	struct
	{
		CartHash hash;
		u64 mdate;
	} cart;

	struct
	{
		bool show;
		bool chained;

		union
		{
			struct
			{
				s8 sprites;
				s8 map;
				s8 sfx;
				s8 music;
			} index;

			s8 indexes[COUNT_OF(BankModes)];
		};
	} bank;

	struct
	{
		s32 counter;
		char message[STUDIO_TEXT_BUFFER_WIDTH];
	} popup;

	struct
	{
		char text[STUDIO_TEXT_BUFFER_WIDTH];
	} tooltip;

	struct
	{
		bool record;

		u32* buffer;
		s32 frames;
		s32 frame;

	} video;

	Code*	code;

	struct
	{
		Sprite*	sprite[TIC_EDITOR_BANKS];
		Map*	map[TIC_EDITOR_BANKS];
		Sfx*	sfx[TIC_EDITOR_BANKS];
		Music*	music[TIC_EDITOR_BANKS];
	} banks;

	Console*	console;
	World*		world;
	Dialog*		dialog;
	Surf*		surf;

	tic_net*	net;

	Start*		start;
	Run*		run;
	Menu*		menu;
	Config*		config;

	tic_fs*		fs;

	s32	samplerate;
	tic_font systemFont;
} impl =
{
	.tic80local = NULL,

	.mode = TIC_START_MODE,
	.prevMode = TIC_CODE_MODE,

	.keycodes = 
	{
		tic_key_up,
		tic_key_down,
		tic_key_left,

		tic_key_z,
		tic_key_x,
		tic_key_a,
		tic_key_s,
	},
	.cart = 
	{
		.mdate = 0,
	},

	.dialogMode = TIC_CONSOLE_MODE,

	.bank = 
	{
		.show = false,
		.chained = true,
	},

	.popup =
	{
		.counter = 0,
		.message = "\0",
	},

	.tooltip = 
	{
		.text = "\0",
	},

	.video = 
	{
		.record = false,
		.buffer = NULL,
		.frames = 0,
	},
};

//#712
const StudioConfig* getConfig()
{
	return &impl.config->data;
}

//#965
static void exitConfirm(bool yes, void* data)
{
	impl.studio.quit = yes;
}

//#970
void exitStudio()
{
	if (impl.mode != TIC_START_MODE && studioCartChanged())
	{
		static const char* Rows[] =
		{
			"YOU HAVE",
			"UNSAVED CHANGES",
			"",
			"DO YOU REALLY WANT",
			"TO EXIT?",
		};
		showDialog(Rows, COUNT_OF(Rows), exitConfirm, NULL);
	}
	else
		exitConfirm(true, NULL);
}

//#1217
void showDialog(const char** text, s32 rows, DialogCallback callback, void* data)
{
	if (impl.mode != TIC_DIALOG_MODE)
	{
		initDialog(impl.dialog, impl.studio.tic, text, rows, callback, data);
		impl.dialogMode = impl.mode;
		setStudioMode(TIC_DIALOG_MODE);
	}
}

//#1627
static void updateStudioProject()
{
	if (impl.mode != TIC_START_MODE)
	{
		Console* console = impl.console;

		u64 date = fs_date(console->rom.path);

		if (impl.cart.mdate && date > impl.cart.mdate)
		{
			if (studioCartChanged())
			{
				static const char* Rows[] = 
				{
					"",
					"CART HAS CHANGED!",
					"",
					"DO YOU WANT",
					"TO RELOAD IT?"
				};
				// TODO
				//showDialog(Rows, COUNT_OF(Rows), reloadConfirm, NULL);
			}
			else console->updateProject(console);
		}
	}
}

//#1891
static void studioTick()
{
	tic_mem* tic = impl.studio.tic;

	tic_net_start (impl.net);

	//TODO
	//processShortcuts();
	//processMouseStates();
	//processGamepadMapping();

	//renderStudio();

	{
		tic_scanline scanline = NULL;
		tic_overline overline = NULL;
		void* data = NULL;

		switch (impl.mode)
		{
			case TIC_MENU_MODE:
			{
				overline = impl.menu->overline;
				scanline = impl.menu->scanline;
				data = impl.menu;
			}
			break;
			case TIC_SPRITE_MODE:
			{
				Sprite* sprite = impl.banks.sprite[impl.bank.index.sprites];
				overline = sprite->overline;
				scanline = sprite->scanline;
				data = sprite;
			}
			break;
			case TIC_MAP_MODE:
			{
				Map* map = impl.banks.map[impl.bank.index.map];
				overline = map->overline;
				scanline = map->scanline;
				data = map;
			}
			break;
			case TIC_WORLD_MODE:
			{
				overline = impl.world->overline;
				scanline = impl.world->scanline;
				data = impl.world;
			}
			break;
			case TIC_DIALOG_MODE:
			{
				overline = impl.dialog->overline;
				scanline = impl.dialog->scanline;
				data = impl.dialog;
			}
			break;
			case TIC_SURF_MODE:
			{
				overline = impl.surf->overline;
				scanline = impl.surf->scanline;
				data = impl.surf;
			}
			break;
		
		default:
			break;
		}

		if (impl.mode != TIC_RUN_MODE)
		{
			memcpy(tic->ram.vram.palette.data, getConfig()->cart->bank0.palette.scn.data, sizeof(tic_palette));
			memcpy(tic->ram.font.data, impl.systemFont.data, sizeof(tic_font));
		}

		data
			? tic_core_blit_ex(tic, tic->screen_format, scanline, overline, data)
			: tic_core_blit(tic, tic->screen_format);

		//TODO
		//if (isRecordFrame())
		//	recordFrame(tic->screen);
	}

	//drawPopup();
	tic_net_end(impl.net);
}

//#1984
static void studioClose()
{
	for (s32 i = 0; i < TIC_EDITOR_BANKS; i++)
	{
		freeSprite	(impl.banks.sprite[i]);
		freeMap		(impl.banks.map[i]);
		freeSfx		(impl.banks.sfx[i]);
		freeMusic	(impl.banks.music[i]);
	}

	freeCode		(impl.code);
	freeConsole		(impl.console);
	freeWorld		(impl.world);
	freeDialog		(impl.dialog);
	freeSurf		(impl.surf);

	freeStart		(impl.start);
	freeRun			(impl.run);
	freeConfig		(impl.config);
	freeMenu		(impl.menu);

	if (impl.tic80local)
		tic80_delete((tic80*)impl.tic80local);

	tic_net_close(impl.net);

	free(impl.fs);
}

//#2019
static StartArgs parseArgs(s32 argc, char **argv)
{
	static const char *const usage[] =
	{
		"tic80 [cart] [options]",
		NULL,
	};

	StartArgs args = {0};
/*
	struct argparse_option options[] =
	{
		OPT_HELP(),
#define CMD_PARAMS_DEF(name, type, post, help) OPT_##type('\0', #name, &args.name, help),
		CMD_PARAMS_LIST(CMD_PARAMS_DEF)
#undef CMD_PARAMS_DEF
		OPT_END(),		
	};

	struct argparse argparse;
	argparse_init(&argparse, options, usage, 0);
	argparse_describe(&argparse, "\n", TIC_NAME " startup options:", NULL);
	argc = argparse_parse(&argparse, argc, (const char**)argv);

	if (argc == 1)
		args.cart = argv[0];
*/
	return args;
	
}

//#2049
Studio* studioInit(s32 argc, char **argv, s32 samplerate, const char* folder)
{
	setbuf(stdout, NULL);

	StartArgs args = parseArgs(argc, argv);

	impl.samplerate = samplerate;

	impl.net = tic_net_create("http://"TIC_HOST);

	{
		const char *path = args.fs ? args.fs : folder;

		if (fs_exists(path))
		{
			impl.fs = tic_fs_create(path, impl.net);
		}
		else
		{
			fprintf(stderr, "error: folder `%s` does'nt exist\n", path);
			exit(1);
		}
	}

	impl.tic80local = (tic80_local*)tic80_create(impl.samplerate);
	impl.studio.tic = impl.tic80local->memory;

	{
		for (s32 i = 0; i < TIC_EDITOR_BANKS; i++)
		{
			impl.banks.sprite[i]	= calloc(1, sizeof(Sprite));
			impl.banks.map[i] 		= calloc(1, sizeof(Map));
			impl.banks.sfx[i]		= calloc(1, sizeof(Sfx));
			impl.banks.music[i] 	= calloc(1, sizeof(Music));
		}

		impl.code 		= calloc(1, sizeof(Code));
		impl.console 	= calloc(1, sizeof(Console));
		impl.world 		= calloc(1, sizeof(World));
		impl.dialog 	= calloc(1, sizeof(Dialog));
		impl.surf 		= calloc(1, sizeof(Surf));

		impl.start 		= calloc(1, sizeof(Start));
		impl.run		= calloc(1, sizeof(Run));
		impl.menu		= calloc(1, sizeof(Menu));
		impl.config 	= calloc(1, sizeof(Config));
	}

	tic_fs_makedir(impl.fs, TIC_LOCAL);
	tic_fs_makedir(impl.fs, TIC_LOCAL_VERSION);

	initConfig(impl.config, impl.studio.tic, impl.fs);
	initKeymap();
	initStart(impl.start, impl.studio.tic, args.cart);
	initRunMode();

	initConsole(impl.console, impl.studio.tic, impl.fs, impl.net, impl.config, args);
	initSurfMode();
	initModules();

	if (args.scale)
		impl.config->data.uiScale = args.scale;

	impl.config->data.goFullscreen = args.fullscreen;
	impl.config->data.noSound = args.nosound;
	impl.config->data.cli = args.cli;

	impl.studio.tick = studioTick;
	impl.studio.close = studioClose;
	impl.studio.updateProject = updateStudioProject;
	impl.studio.exit = exitStudio;
	impl.studio.config = getConfig;

	if (args.cli)
		args.skip = true;

	if (args.skip)
		setStudioMode(TIC_CONSOLE_MODE);

	return &impl.studio;
}
