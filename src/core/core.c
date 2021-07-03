#include "api.h"
#include "core.h"

#include <string.h>
#include <stdlib.h>

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
