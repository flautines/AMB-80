#include "api.h"
#include "core.h"

#include <string.h>
#include <assert.h>

#define ENVELOPE_FREQ_SCALE 2
#define SECONDS_PER_MINUTE 60
#define NOTES_PER_MINUTE (TIC80_FRAMERATE / NOTES_PER_BEAT * SECONDS_PER_MINUTE)
#define PIANO_START 8

static const u16 NoteFreqs[] = { 0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17, 0x18, 0x1a,  0x1c, 0x1d, 0x1f, 0x21, 0x23, 0x25, 0x27, 0x29, 0x2c, 0x2e, 0x31, 0x34, 0x37, 0x3a, 0x3e, 0x41, 0x45, 0x49, 0x4e, 0x52, 0x57, 0x5c, 0x62, 0x68, 0x6e, 0x75, 0x7b, 0x83, 0x8b, 0x93, 0x9c, 0xa5, 0xaf, 0xb9, 0xc4, 0xd0, 0xdc, 0xe9, 0xf7, 0x106, 0x115, 0x126, 0x137, 0x14a, 0x15d, 0x172, 0x188, 0x19f, 0x1b8, 0x1d2, 0x1ee, 0x20b, 0x22a, 0x24b, 0x26e, 0x293, 0x2ba, 0x2e4, 0x310, 0x33f, 0x370, 0x3a4, 0x3dc, 0x417, 0x455, 0x497, 0x4dd, 0x527, 0x575, 0x5c8, 0x620, 0x67d, 0x6e0, 0x749, 0x7b8, 0x82d, 0x8a9, 0x92d, 0x9b9, 0xa4d, 0xaea, 0xb90, 0xc40, 0xcfa, 0xdc0, 0xe91, 0xf6f, 0x105a, 0x1153, 0x125b, 0x1372, 0x149a, 0x15d4, 0x1720, 0x1880 };
static_assert(COUNT_OF(NoteFreqs) == NOTES * OCTAVES + PIANO_START , "count_of_freqs");
static_assert(sizeof(tic_sound_register) == 16 + 2                 , "tic_sound_register");
static_assert(sizeof(tic_sample) == 66                             , "tic_sample");
static_assert(sizeof(tic_track_pattern) == 3 * MUSIC_PATTERN_ROWS  ,"tic_sample");
static_assert(sizeof(tic_track) == 3 * MUSIC_FRAMES + 3            , "tic_track");
static_assert(tic_music_cmd_count == 1 << MUSIC_CMD_BITS           , "tic_music_cmd_count");
static_assert(sizeof(tic_music_state) == 4                         , "tic_music_state_size");

static s32 getTempo(tic_core* core, const tic_track* track)
{
    return core->state.music.tempo < 0
        ? track->tempo + DEFAULT_TEMPO
        : core->state.music.tempo;
}

static s32 getSpeed(tic_core* core, const tic_track* track)
{
  return core->state.music.speed < 0
    ? track->speed + DEFAULT_SPEED
    : core->state.music.speed;
}
static s32 row2tick(tic_core* core, const tic_track* track, s32 row)
{
    s32 tempo = getTempo(core, track);
    return tempo
        ? row * getSpeed(core, track) * NOTES_PER_MINUTE / tempo / DEFAULT_SPEED
        : 0;
}

static void resetSfxPos(tic_channel_data* channel)
{
    memset(channel->pos->data, -1, sizeof(tic_sfx_pos));
    channel->tick = -1;
}

static void setChannelData(tic_mem* memory, s32 index, s32 note, s32 octave, s32 duration, tic_channel_data* channel, s32 volumeLeft, s32 volumeRight, s32 speed)
{
    tic_core* core = (tic_core*)memory;

    channel->volume.left = volumeLeft;
    channel->volume.right = volumeRight;

    if (index >= 0)
    {
        struct { s8 speed : SFX_SPEED_BITS; } temp = { speed };
        channel->speed = speed == temp.speed ? speed : memory->ram.sfx.samples.data[index].speed;

        channel->note = note + octave * NOTES;
        channel->duration = duration;
        channel->index = index;

        resetSfxPos(channel);
    }
}

static void setMusicChannelData(tic_mem* memory, s32 index, s32 note, s32 octave, s32 left, s32 right, s32 channel)
{
    tic_core* core = (tic_core*)memory;
    setChannelData(memory, index, note, octave, -1, &core->state.music.channels[channel], left, right, SFX_DEF_SPEED);
}

static void resetMusicChannels(tic_mem* memory)
{
    for (s32 c = 0; c < TIC_SOUND_CHANNELS; c++)
        setMusicChannelData(memory, -1, 0, 0, 0, 0, c);

    tic_core* core = (tic_core*)memory;
    memset(core->state.music.commands, 0, sizeof core->state.music.commands);
    memset(&core->state.music.jump, 0, sizeof(tic_jump_command));
}

static void setMusic(tic_core* core, s32 index, s32 frame, s32 row, bool loop, bool sustain, s32 tempo, s32 speed)
{
    tic_mem* memory = (tic_mem*)core;
    tic_ram* ram = &memory->ram;

    ram->music_state.music.track = index;

    if (index < 0) 
    {
        ram->music_state.flag.music_status = tic_music_stop;
        resetMusicChannels(memory);
    }
    else
    {
        for (s32 c = 0; c < TIC_SOUND_CHANNELS; c++)
            setMusicChannelData(memory, -1, 0, 0, MAX_VOLUME, MAX_VOLUME, c);

        ram->music_state.music.row = row;
        ram->music_state.music.frame = frame < 0 ? 0 : frame;
        ram->music_state.flag.music_loop = loop;
        ram->music_state.flag.music_sustain = sustain;
        ram->music_state.flag.music_status = tic_music_play;

        const tic_track* track = &ram->music.tracks.data[index];
        core->state.music.tempo = tempo;
        core->state.music.speed = speed;
        core->state.music.ticks = row >= 0 ? row2tick(core, track, row) : 0;
    }
}

void tic_api_music(tic_mem* memory, s32 index, s32 frame, s32 row, bool loop, bool sustain, s32 tempo, s32 speed)
{
    tic_core* core = (tic_core*)memory;

    setMusic(core, index, frame, row, loop, sustain, tempo, speed);

    if (index >= 0)
        memory->ram.music_state.flag.music_status = tic_music_play;    
}
