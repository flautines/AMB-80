#include "api.h"
#include "core.h"
#include "tilesheet.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <time.h>
#include <assert.h>

static_assert(TIC_BANK_BITS == 3,                 "tic_bank_bits");
static_assert(sizeof(tic_map) < 1024 * 32,        "tic_map");
static_assert(sizeof(tic_vram) == TIC_VRAM_SIZE,  "tic_vram");
static_assert(sizeof(tic_ram) == TIC_RAM_SIZE,    "tic_ram");

static void setPixelDma(tic_mem* tic, s32 x, s32 y, u8 color)
{
	tic_tool_poke4(tic->ram.vram.screen.data, y * TIC80_WIDTH + x, color);
}

static u8 getPixelDma(tic_mem* tic, s32 x, s32 y)
{
	tic_core* core = (tic_core*)tic;
	return tic_tool_peek4(core->memory.ram.vram.screen.data, y * TIC80_WIDTH + x);
}

static void drawHLineDma(tic_mem* memory, s32 xl, s32 xr, s32 y, u8 color)
{
	color = color << 4 | color;

	if (xl >= xr) return;

	if (xl & 1)
	{
		tic_tool_poke4(&memory->ram.vram.screen.data, y * TIC80_WIDTH + xl, color);
		xl++;
	}
	s32 count = (xr - xl) >> 1;
	u8* screen = memory->ram.vram.screen.data + ((y * TIC80_WIDTH + xl) >> 1);
	for (s32 i = 0; i < count; i++)
		*screen++ = color;
	if (xr & 1)
  	{
		tic_tool_poke4(&memory->ram.vram.screen.data, y * TIC80_WIDTH + xr - 1, color);
  	}
}

static void resetPalette(tic_mem* memory)
{
	static const u8 DefaultMapping[] = { 16, 50, 84, 118, 152, 186, 220, 254 };
	memcpy(memory->ram.vram.palette.data, memory->cart.bank0.palette.scn.data, sizeof(tic_palette));
	memcpy(memory->ram.vram.mapping, DefaultMapping, sizeof DefaultMapping);
}

static void updateSaveid(tic_mem* memory)
{
	memset(memory->saveid, 0, sizeof memory->saveid);
	const char* saveid = tic_tool_metatag(memory->cart.code.data, "saveid", tic_core_script_config(memory)->singleComment);
	if (saveid)
	{
		strncpy(memory->saveid, saveid, TIC_SAVEID_SIZE - 1);
		free((void*)saveid);
	}
}

static void soundClear(tic_mem* memory)
{
	tic_core* core = (tic_core*)memory;
	for (s32 i = 0; i < TIC_SOUND_CHANNELS; i++) {
		static const tic_channel_data EmptyChannel = 
		{
			.tick = -1,
			.pos = NULL,
			.index = -1,
			.note = 0,
			.volume = {0, 0},
			.speed = 0,
			.duration = -1,
		};

		memcpy(&core->state.music.channels[i], &EmptyChannel, sizeof EmptyChannel);
		memcpy(&core->state.sfx.channels[i], &EmptyChannel, sizeof EmptyChannel);

		memset(core->state.sfx.channels[i].pos = &memory->ram.sfxpos[i], -1, sizeof(tic_sfx));
		memset(core->state.music.channels[i].pos = &core->state.music.sfxpos[i], -1, sizeof(tic_sfx_pos));
	}

	memset(&memory->ram.registers, 0, sizeof memory->ram.registers);
	memset(&memory->samples.buffer, 0, memory->samples.size);

	tic_api_music(memory, -1, 0, 0, false, false, -1, -1);
}

static void resetBlitSegment(tic_mem* memory)
{
	memory->ram.vram.blit.segment = TIC_DEFAULT_BLIT_MODE;
}

//#163
static inline void sync(void* dst, void* src, s32 size, bool rev)
{
	if (rev)
		SWAP(dst, src, void*);

	memcpy(dst, src, size);
}

//#171
void tic_api_sync(tic_mem* tic, u32 mask, s32 bank, bool toCart)
{
	tic_core* core = (tic_core*)tic;

	static const struct { s32 bank; s32 ram; s32 size; u8 mask; } Sections[] =
	{
#define TIC_SYNC_DEF(CART, RAM, ...) { offsetof(tic_bank, CART), offsetof(tic_ram, RAM), sizeof(tic_##CART), tic_sync_##CART },
		TIC_SYNC_LIST(TIC_SYNC_DEF)
#undef	TIC_SYNC_DEF		
	};

	enum { Count = COUNT_OF(Sections), Mask = (1 << Count) - 1 };

	if (mask == 0) mask = Mask;

	mask &= ~core->state.synced & Mask;

	assert(bank >= 0 && bank < TIC_BANKS);

	for (s32 i = 0; i < Count; i++)
		if (mask & Sections[i].mask)
			sync((u8*)&tic->ram + Sections[i].ram, (u8*)&tic->cart.banks[bank] + Sections[i].bank, Sections[i].size, toCart);

	// copy OVR palette
	if (mask & tic_sync_palette)
		sync(&core->state.ovr.palette, &tic->cart.banks[bank].palette.ovr, sizeof(tic_palette), toCart);

	core->state.synced |= mask;
}

//#253
static bool compareMetatag(const char* code, const char* tag, const char* value, const char* comment)
{
	bool result = false;

	const char* str = tic_tool_metatag(code, tag, comment);

	if (str)
	{
		result = strcmp(str, value) == 0;
		free((void*)str);
  	}

	  return result;
}
extern const tic_script_config* get_lua_script_config();

const tic_script_config* tic_core_script_config(tic_mem* memory)
{
	static const struct Config
  	{
		const char* name;
		const tic_script_config*(*func)();    
  	} Configs[] =
	{
#define SCRIPT_DEF(name, ...) {#name, get_## name ##_script_config},
		SCRIPT_LIST(SCRIPT_DEF)
#undef  SCRIPT_DEF    
	};

  	FOR(const struct Config*, it, Configs)
		if(compareMetatag(memory->cart.code.data, "script", it->name, it->func()->singleComment))
	  	return it->func();

	return Configs->func();
}

static void resetDma(tic_mem* memory)
{
	tic_core* core = (tic_core*)memory;
	core->state.setpix = setPixelDma;
	core->state.getpix = getPixelDma;
	core->state.drawhline = drawHLineDma;
}

//#341
void tic_api_reset(tic_mem* memory)
{
	resetPalette(memory);
	resetBlitSegment(memory);

	memset(&memory->ram.vram.vars, 0, sizeof memory->ram.vram.vars);

	tic_api_clip(memory, 0, 0, TIC80_WIDTH, TIC80_HEIGHT);

	soundClear(memory);

	tic_core* core = (tic_core*)memory;
	core->state.initialized = false;
	core->state.scanline = NULL;
	core->state.ovr.callback = NULL;

	resetDma(memory);

	updateSaveid(memory);
}

//#362
static void cart2ram(tic_mem* memory)
{
	static const u8 Font[] =
	{
		#include "font.inl"
	};

	memcpy(memory->ram.font.data, Font, sizeof Font);

	enum
	{
#define		TIC_SYNC_DEF(NAME, _, INDEX) sync_##NAME = INDEX,
			TIC_SYNC_LIST(TIC_SYNC_DEF)
#undef		TIC_SYNC_DEF
		count,
		all = (1 << count) - 1,
		noscreen = BITCLEAR(all, sync_screen)
	};

	tic_api_sync(memory, EMPTY(memory->cart.bank0.screen.data) ? noscreen : all, 0, false);
}

//#385
void tic_core_tick(tic_mem* tic, tic_tick_data* data)
{
	tic_core* core = (tic_core*)tic;

	core->data = data;

	if (!core->state.initialized)
	{
		const char* code = tic->cart.code.data;

		bool done = false;
		const tic_script_config* config = tic_core_script_config(tic);

		if (strlen(code))
		{
			cart2ram(tic);

			core->state.synced = 0;
			tic->input.data = 0;

			if (compareMetatag(code, "input", "mouse", config->singleComment))
				tic->input.mouse = 1;
			else if (compareMetatag(code, "input", "gamepad", config->singleComment))
			tic->input.gamepad = 1;
			else if (compareMetatag(code, "input", "keyboard", config->singleComment))
				tic->input.keyboard = 1;
			else tic->input.data = -1;  // default is all enabled

			data->start = data->counter(core->data->data);

			done = config->init(tic, code);
		}
		else
		{
			core->data->error(core->data->data, "the code is empty");
		}

		if (done)
		{
			core->state.tick = config->tick;
			core->state.scanline = config->scanline;
			core->state.ovr.callback = config->overline;

			core->state.initialized = true;
		}
		else return;
	}

	core->state.tick(tic);
}

//#463
void tic_core_close(tic_mem* memory)
{
	tic_core* core = (tic_core*)memory;

	core->state.initialized = false;

#define SCRIPT_DEF(name, ...)	 get_## name ##_script_config()->close(memory);
	SCRIPT_LIST(SCRIPT_DEF)
#undef SCRIPT_DEF

	blip_delete(core->blip.left);
	blip_delete(core->blip.right);

	free(memory->samples.buffer);
	free(core);
}

//#480
void tic_core_tick_start(tic_mem* memory)
{
	tic_core_sound_tick_start(memory);
	tic_core_tick_io(memory);

	tic_core* core = (tic_core*)memory;
	core->state.synced = 0;
	resetDma(memory);
}

//#490
void tic_core_tick_end(tic_mem* memory)
{
	tic_core* core = (tic_core*)memory;
	tic80_input* input = &core->memory.ram.input;

	core->state.gamepads.previous.data = input->gamepads.data;
	core->state.keyboard.previous.data = input->keyboard.data;

	// TODO 
	//tic_core_sound_tick_end(memory);

	//core->state.setpix = setPixelOvr;
	//core->state.getpix = getPixelOvr;
	//core->state.drawhline = drawHLineOvr;
}

// copied from SDL2
static inline void memset4(void* dst, u32 val, u32 dwords)
{
	u32 _n = (dwords + 3) / 4;
	u32* _p = (u32*)dst;
	u32 _val = (val);
	if (dwords == 0) return;

	switch (dwords % 4)
	{
		case 0: do {
			*_p++ = _val;
		case 3:	*_p++ = _val;
		case 2: *_p++ = _val;
		case 1:	*_p++ = _val;
		} while (--_n);
	}
}

//#535
void tic_core_blit_ex(tic_mem* tic, tic80_pixel_color_format fmt, tic_scanline scanline, tic_overline overline, void* data)
{
	// init OVR palette
	{
		tic_core* core = (tic_core*)tic;

		const tic_palette* pal = EMPTY(core->state.ovr.palette.data)
			? &tic->ram.vram.palette
			: &core->state.ovr.palette;

		memcpy(core->state.ovr.raw, tic_tool_palette_blit(pal, fmt), sizeof core->state.ovr.raw);		
	}

	if (scanline)
		scanline(tic, 0, data);

	const u32* pal = tic_tool_palette_blit(&tic->ram.vram.palette, fmt);

	u32* out = tic->screen;

	memset4(&out[0 * TIC80_FULLWIDTH], pal[tic->ram.vram.vars.border], TIC80_FULLWIDTH * TIC80_MARGIN_TOP);

	u32* rowPtr = out + (TIC80_MARGIN_TOP * TIC80_FULLWIDTH);
	for (s32 r = 0; r < TIC80_HEIGHT; r++, rowPtr += TIC80_FULLWIDTH)
	{
		u32* colPtr = rowPtr + TIC80_MARGIN_LEFT;
		memset4(rowPtr, pal[tic->ram.vram.vars.border], TIC80_MARGIN_LEFT);

		s32 pos = (r + tic->ram.vram.vars.offset.y + TIC80_HEIGHT) % TIC80_HEIGHT * TIC80_WIDTH >> 1;

		u32 x = (-tic->ram.vram.vars.offset.x + TIC80_WIDTH) % TIC80_WIDTH;
		for (s32 c = 0; c < TIC80_WIDTH / 2; c++)
		{
			u8 val = ((u8*)tic->ram.vram.screen.data)[pos + c];
			*(colPtr + (x++ % TIC80_WIDTH)) = pal[val & 0xf];
			*(colPtr + (x++ % TIC80_WIDTH)) = pal[val >> 4];
		}

		memset4(rowPtr + (TIC80_FULLWIDTH - TIC80_MARGIN_RIGHT), pal[tic->ram.vram.vars.border], TIC80_MARGIN_RIGHT);

		if (scanline && (r < TIC80_HEIGHT - 1))
		{
			scanline(tic, r + 1, data);
			pal = tic_tool_palette_blit(&tic->ram.vram.palette, fmt);
		}

		memset4(&out[(TIC80_FULLHEIGHT - TIC80_MARGIN_BOTTOM) * TIC80_FULLWIDTH], pal[tic->ram.vram.vars.border], TIC80_FULLWIDTH * TIC80_MARGIN_BOTTOM);

		if (overline)
			overline(tic, data);
	}
}

//#589
static inline void scanline(tic_mem* memory, s32 row, void* data)
{
	tic_core* core = (tic_core*)memory;

	if (core->state.initialized)
		core->state.scanline(memory, row, data);
}

//#597
static inline void overline(tic_mem* memory, void* data)
{
	tic_core* core = (tic_core*)memory;

	if (core->state.initialized)
		core->state.ovr.callback(memory, data);
}
//#605
void tic_core_blit(tic_mem* tic, tic80_pixel_color_format fmt)
{
	tic_core_blit_ex(tic, fmt, scanline, overline, NULL);
}

//#610
tic_mem* tic_core_create(s32 samplerate)
{
	tic_core* core = (tic_core*)malloc(sizeof(tic_core));
	memset(core, 0, sizeof(tic_core));

	if (core != (tic_core*)&core->memory) {
		free(core);
		return NULL;        
	}

	core->memory.screen_format = TIC80_PIXEL_COLOR_RGBA8888;
	core->samplerate = samplerate;

	core->memory.samples.size = samplerate * TIC_STEREO_CHANNELS / TIC80_FRAMERATE * sizeof(s16);
	core->memory.samples.buffer = malloc(core->memory.samples.size);

	core->blip.left = blip_new(samplerate / 10);
	core->blip.right = blip_new(samplerate / 10);

	blip_set_rates(core->blip.left, CLOCKRATE, samplerate);
	blip_set_rates(core->blip.right, CLOCKRATE, samplerate);

	tic_api_reset(&core->memory);

	return &core->memory;
}
