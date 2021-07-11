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

#pragma once

#include "api.h"
#include "version.h"

#define TIC_VERSION_POST ""

#define TIC_VERSION DEF2STR(TIC_VERSION_MAJOR) "." DEF2STR(TIC_VERSION_MINOR) "." DEF2STR(TIC_VERSION_REVISION) TIC_VERSION_STATUS TIC_VERSION_BUILD TIC_VERSION_POST " (" TIC_VERSION_HASH ")"
#define TIC_PACKAGE "com.amb.tic"
#define TIC_NAME "AMB-80"
#define TIC_NAME_FULL TIC_NAME " tiny computer"
#define TIC_TITLE TIC_NAME_FULL " " TIC_VERSION
#define TIC_HOST "tic80.com"
#define TIC_WEBSITE "https://" TIC_HOST
#define TIC_COPYRIGHT TIC_WEBSITE " (C) 2017-" TIC_VERSION_YEAR

#define TICNAME_MAX 256

#define CODE_COLORS_LIST(macro) \
	macro(BG)	\
	macro(FG)	\
	macro(STRING)	\
	macro(NUMBER)	\
	macro(KEYWORD)	\
	macro(API)		\
	macro(COMMENT)	\
	macro(SIGN)

typedef struct
{
	struct
	{
		struct
		{
			s32 arrow;
			s32 hand;
			s32 ibeam;

			bool pixelPerfect;
		} cursor;

		struct
		{
#define		CODE_COLOR_DEF(VAR) u8 VAR;
			CODE_COLORS_LIST(CODE_COLOR_DEF)
#undef		CODE_COLOR_DEF

			u8 select;
			u8 cursor;
			bool shadow;
			bool altFont;
			bool matchDelimiters;

		} code;

		struct 
		{
			struct
			{
				u8 alpha;
			} touch;

		} gamepad;
		
	} theme;

	s32 gifScale;
	s32 gifLength;

	bool checkNewVersion;
	bool noSound;
	bool cli;

	bool goFullscreen;

	const tic_cartridge* cart;

	s32 uiScale;

} StudioConfig;

typedef struct
{
	tic_mem* tic;
	bool quit;

	void (*tick)();
	void (*exit)();
	void (*close)();
	void (*updateProject)();
	const StudioConfig* (*config)();

} Studio;

Studio* studioInit(s32 argc, char **arv, s32 samplerate, const char* appFolder);
