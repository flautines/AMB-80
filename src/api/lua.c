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
