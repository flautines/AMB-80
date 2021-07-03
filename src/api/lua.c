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

#include "core/core.h"

#if defined(TIC_BUILD_WITH_LUA)

#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <ctype.h>

#define LUA_LOC_STACK 100000000

static const char TicCore[] = "_TIC80";

//#41
static s32 getLuaNumber(lua_State* lua, s32 index)
{
  return (s32)lua_tonumber(lua, index);
}

//#46
static void registerLuaFunction(tic_core* core, lua_CFunction func, const char *name)
{
  lua_pushcfunction(core->lua, func);
  lua_setglobal(core->lua, name);
}

//#52
static tic_core* getLuaCore(lua_State* lua)
{
  lua_getglobal(lua, TicCore);
  tic_core* core = lua_touserdata(lua, -1);
  lua_pop(lua, 1);
  return core;
}

//#60 TODO
static s32 lua_peek(lua_State* lua) {}

//#76 TODO
static s32 lua_poke(lua_State* lua) {}

//#93 TODO
static s32 lua_peek4(lua_State* lua) {}

//#109 TODO
static s32 lua_poke4(lua_State* lua) {}

//#126 TODO
static s32 lua_cls(lua_State* lua) {}

//#137 TODO
static s32 lua_pix(lua_State* lua) {}

//#165 TODO
static s32 lua_line(lua_State* lua) {}

//#186 TODO
static s32 lua_rect(lua_State* lua) {}

//#207 TODO
static s32 lua_rectb(lua_State* lua) {}

//#228 TODO
static s32 lua_circ(lua_State* lua) {}

//#248 TODO
static s32 lua_circb(lua_State* lua) {}

//#268 TODO
static s32 lua_elli(lua_State* lua) {}

//#289 TODO
static s32 lua_ellib(lua_State* lua) {}

//#310 TODO
static s32 lua_tri(lua_State* lua) {}

//#332 TODO
static s32 lua_trib(lua_State* lua) {}

//#354 TODO
static s32 lua_textri(lua_State* lua) {}

//#415 TODO
static s32 lua_clip(lua_State* lua) {}

//#441 TODO
static s32 lua_btnp(lua_State* lua) {}

//#475 TODO
static s32 lua_btn(lua_State* lua) {}

//#499 TODO
static s32 lua_spr(lua_State* lua) {}

//#580 TODO
static s32 lua_mget(lua_State* lua) {}

//#600 TODO
static s32 lua_mset(lua_State* lua) {}

//#641 TODO
static s32 lua_map(lua_State* lua) {}

//#730 TODO
static s32 lua_music(lua_State* lua) {}

//#786 TODO
static s32 lua_sfx(lua_State* lua) {}

//#875 TODO
static s32 lua_sync(lua_State* lua) {}

//#906 TODO
static s32 lua_reset(lua_State* lua) {}

//#915 TODO
static s32 lua_key(lua_State* lua) {}

//#947 TODO
static s32 lua_keyp(lua_State* lua) {}

//#990 TODO
static s32 lua_memcpy(lua_State* lua) {}

//#1026
static const char* printString(lua_State* lua, s32 index)
{
  lua_getglobal(lua, "tostring");
  lua_pushvalue(lua, -1);
  lua_pushvalue(lua, index);
  lua_call(lua, 1, 1);

  const char* text = lua_tostring(lua, -1);

  lua_pop(lua, 2);

  return text;
}

//#1040 TODO
static s32 lua_font(lua_State* lua) {}

//#1105
static s32 lua_print(lua_State* lua)
{
  s32 top = lua_gettop(lua);

  if (top >= 1)
  {
    tic_mem* tic = (tic_mem*)getLuaCore(lua);

    s32 x = 0;
    s32 y = 0;
    s32 color = TIC_DEFAULT_COLOR;
    bool fixed = false;
    s32 scale = 1;
    bool alt = false;

    const char* text = printString(lua, 1);

    if (top >= 3)
    {
      x = getLuaNumber(lua, 2);
      y = getLuaNumber(lua, 3);

      if (top >= 4)
      {
        color = getLuaNumber(lua, 4) % TIC_PALETTE_SIZE;

        if (top >= 5)
        {
          fixed = lua_toboolean(lua, 5);

          if (top >= 6)
          {
            scale = getLuaNumber(lua, 6);

            if (top >= 7)
            {
              alt = lua_toboolean(lua, 7);
            }
          }
        }
      }
    }

    if (scale == 0)
    {
      lua_pushinteger(lua, 0);
      return 1;
    }

    s32 size = tic_api_print(tic, text ? text : "nil", x, y, color, fixed, scale, alt);

    lua_pushinteger(lua, size);

    return 1;
  }

  return 0;
}

//#1008 TODO
static s32 lua_memset(lua_State* lua) {}

//#1164 TODO
static s32 lua_trace(lua_State* lua) {}

//#1186 TODO
static s32 lua_pmem(lua_State* lua) {}

//#1217 TODO
static s32 lua_time(lua_State* lua) {}

//#1226 TODO
static s32 lua_tstamp(lua_State* lua) {}

//#1235 TODO
static s32 lua_exit(lua_State* lua) {}

//#1242 TODO
static s32 lua_mouse(lua_State* lua) {}

//#1264 TODO
static s32 lua_fget(lua_State* lua) {}

//#1286 TODO
static s32 lua_fset(lua_State* lua) {}

//#1313
static s32 lua_dofile(lua_State *lua)
{
  luaL_error(lua, "unknown method: \"dofile\"\n");

  return 0;
}

//#1320
static s32 lua_loadfile(lua_State *lua)
{
  luaL_error(lua, "unknown method: \"loadfile\"\n");

  return 0;
}

//#1327
static void lua_open_builtins(lua_State *lua)
{
  static const luaL_Reg loadedlibs[] = 
  {
    { "_G", luaopen_base },
    { LUA_LOADLIBNAME , luaopen_package   },
    { LUA_COLIBNAME   , luaopen_coroutine },
    { LUA_TABLIBNAME  , luaopen_table     },
    { LUA_STRLIBNAME  , luaopen_string    },
    { LUA_MATHLIBNAME , luaopen_math      },
    { LUA_DBLIBNAME   , luaopen_debug     },
    { NULL, NULL }
  };

  for (const luaL_Reg *lib = loadedlibs; lib->func; lib++)
  {
    luaL_requiref(lua, lib->name, lib->func, 1);
    lua_pop(lua, 1);
  }
}

//#1348
static void checkForceExit(lua_State *lua, lua_Debug *luadebug)
{
  tic_core* core = getLuaCore(lua);

  tic_tick_data* tick = core->data;

  if (tick->forceExit && tick->forceExit(tick->data))
    luaL_error(lua, "script execution was interrupted");
}

//#1358
static void initAPI(tic_core* core)
{
  lua_pushlightuserdata(core->lua, core);
  lua_setglobal(core->lua, TicCore);

#define API_FUNC_DEF(name, ...)   {lua_ ## name, #name},
  static const struct{lua_CFunction func; const char* name;} ApiItems[] = {TIC_API_LIST(API_FUNC_DEF)};
#undef API_FUNC_DEF

  for (s32 i = 0; i < COUNT_OF(ApiItems); i++)
    registerLuaFunction(core, ApiItems[i].func, ApiItems[i].name);

  registerLuaFunction(core, lua_dofile, "dofile");
  registerLuaFunction(core, lua_loadfile, "loadfile");

  lua_sethook(core->lua, &checkForceExit, LUA_MASKCOUNT, LUA_LOC_STACK);
}

//#1376
static void closeLua(tic_mem* tic)
{
  tic_core* core = (tic_core*)tic;

  if (core->lua)
  {
    lua_close(core->lua);
    core->lua = NULL;
  }
}

//#1387
static bool initLua(tic_mem* tic, const char* code)
{
  tic_core* core = (tic_core*)tic;

  closeLua(tic);

  lua_State* lua = core->lua = luaL_newstate();
  lua_open_builtins(lua);

  initAPI(core);

  {
    lua_State* lua = core->lua;

    lua_settop(lua, 0);

    if (luaL_loadstring(lua, code) != LUA_OK || lua_pcall(lua, 0, LUA_MULTRET, 0) != LUA_OK)
    {
      core->data->error(core->data->data, lua_tostring(lua, -1));
      return false;
    }
  }

  return true;
}

/*
** Message handler which appends stract trace to exceptions.
** This function was extractred from lua.c.
*/
static s32 msghandler (lua_State *lua)
{
  const char *msg = lua_tostring(lua, 1);
  if (msg == NULL) /* is error object not a string? */
  {
    if (luaL_callmeta(lua, 1, "__tostring") &&  /* does it have a metamethod */
      lua_type(lua, -1) == LUA_TSTRING)         /* that produces a string?   */
      return 1; /* that is the message */
    else
      msg = lua_pushfstring(lua, "(error object is a %s value)", luaL_typename(lua, 1));
  }
  luaL_traceback(lua, lua, msg, 1); /* append a standard traceback */
  return 1; /* return the traceback */
}

/*
** Interface to 'lua_pcall', which sets appropriate message handler function.
** Please use this function for all top level lua functions.
** This function was extracted from lua.c (and stripped of signal handling)
*/
//#1437
static s32 docall (lua_State *lua, s32 narg, s32 nres)
{
  s32 status = 0;
  s32 base = lua_gettop(lua) - narg;
  lua_pushcfunction(lua, msghandler);
  lua_insert(lua, base);
  status = lua_pcall(lua, narg, nres, base);
  lua_remove(lua, base);
  return status;
}

//#1448
static void callLuaTick(tic_mem* tic)
{
  tic_core* core = (tic_core*)tic;

  lua_State* lua = core->lua;

  if (lua)
  {
    lua_getglobal(lua, TIC_FN);
    if (lua_isfunction(lua, -1))
    {
      if (docall(lua, 0, 0) != LUA_OK)
        core->data->error(core->data->data, lua_tostring(lua, -1));
    }
    else
    {
      lua_pop(lua, 1);
      core->data->error(core->data->data, "'function TIC() ...' isn't found :(");
    }
  }
}

//#1470
static void callLuaScanlineName(tic_mem* tic, s32 row, void* data, const char* name)
{
  tic_core* core = (tic_core*)tic;
  lua_State* lua = core->lua;

  if (lua)
  {
    lua_getglobal(lua, name);
    if (lua_isfunction(lua, -1))
    {
      lua_pushinteger(lua, row);
      if (docall(lua, 1, 0) == LUA_OK)
        core->data->error(core->data->data, lua_tostring(lua, -1));
    }
    else lua_pop(lua, 1);
  }
}

//#1488
static void callLuaScanline(tic_mem* tic, s32 row, void* data)
{
  callLuaScanlineName(tic, row, data, SCN_FN);

  // try to call old scanline
  callLuaScanlineName(tic, row, data, "scanline");
}

//#1496
static void callLuaOverline(tic_mem* tic, void* data)
{
  tic_core* core = (tic_core*)tic;
  lua_State* lua = core->lua;

  if (lua)
  {
    const char* OvrFunc = OVR_FN;

    lua_getglobal(lua, OvrFunc);
    if (lua_isfunction(lua, -1))
    {
      if (docall(lua, 0, 0) != LUA_OK)
        core->data->error(core->data->data, lua_tostring(lua, -1));
    }
    else lua_pop(lua, 1);
  }
}

//#1516
static const char* const LuaKeywords [] =
{
  "and", "break", "do", "else", "elseif",
  "end", "false", "for", "function", "goto", "if",
  "in", "local", "nil", "not", "or", "repeat",
  "return", "then", "true", "until", "while"
};

static inline bool isalnum_(char c) {return isalnum(c) || c == '_';}

static const tic_outline_item* getLuaOutline(const char* code, s32* size)
{
  enum{Size = sizeof(tic_outline_item)};

  *size = 0;

  static tic_outline_item* items = NULL;

  if (items)
  {
    free(items);
    items = NULL;
  }

  const char* ptr = code;

  while (true)
  {
    static const char FuncString[] = "function ";

    ptr = strstr(ptr, FuncString);

    if (ptr)
    {
      ptr += sizeof FuncString - 1;

      const char* start = ptr;
      const char* end = start;

      while (*ptr)
      {
        char c = *ptr;

        if (isalnum_(c) || c == ':');
        else if (c == '(')
        {
          end = ptr;
          break;
        }
        else break;

        ptr++;
      }

      if (end > start)
      {
        items = realloc(items, (*size + 1) * Size);

        items[*size].pos = start;
        items[*size].size = (s32)(end - start);

        (*size)++;
      }
    }
    else break;
  }

  return items;
}

//#1586
static void evalLua(tic_mem* tic, const char* code)
{
  tic_core* core = (tic_core*)tic;
  lua_State* lua = core->lua;

  if (!lua) return;

  lua_settop(lua, 0);

  if (luaL_loadstring(lua, code) != LUA_OK || lua_pcall(lua, 0, LUA_MULTRET, 0) != LUA_OK)
  {
    core->data->error(core->data->data, lua_tostring(lua, -1));
  }
}

//#1600
static const tic_script_config LuaSyntaxConfig =
{
  .name               = "lua",
  .init               = initLua,
  .close              = closeLua,
  .tick               = callLuaTick,
  .scanline           = callLuaScanline,
  .overline           = callLuaOverline,

  .getOutline         = getLuaOutline,
  .eval               = evalLua,

  .blockCommentStart  = "--[[",
  .blockCommentEnd    = "]]",
  .blockCommentStart2 = NULL,
  .blockCommentEnd2   = NULL,
  .singleComment      = "--",
  .blockStringStart   = "[[",
  .blockStringEnd     = "]]",

  .keywords           = LuaKeywords,
  .keywordsCount      = COUNT_OF(LuaKeywords),  
};

//#1624
const tic_script_config* get_lua_script_config()
{
  return &LuaSyntaxConfig;
}

#endif
