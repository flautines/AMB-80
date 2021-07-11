# SOURCE AND OBJECT CONFIGURATION
CC:=clang
CCFLAGS:=-g -Wall -pedantic -std=c11
LUADIR:=-I/usr/include/lua5.3
INCDIRS:=-Iinclude -Isrc -Ibuild -Ivendor/blip-buf $(shell sdl2-config --cflags) $(LUADIR)
LUALIB:=lua5.3
LDFLAGS:=$(shell sdl2-config --libs) -l$(LUALIB)
SUBDIRS:=src/api src/core src/ext src/studio src/system/sdl src/
OBJSUBDIRS:=$(foreach DIR, $(SUBDIRS), $(patsubst $(SRCDIR)%, $(OBJDIR)%, $(DIR)))
SRCFILES:=$(foreach DIR, $(SUBDIRS), $(wildcard $(DIR)/*.c))
PLAYERFILES:=$(filter-out src/system/sdl/main.o, $(patsubst $(SRCDIR)%, $(OBJDIR)%, $(SRCFILES:%.c=%.o)) \
	vendor/blip-buf/blip_buf.o)
OBJFILES:=$(filter-out src/system/sdl/player.o, $(patsubst $(SRCDIR)%, $(OBJDIR)%, $(SRCFILES:%.c=%.o)) \
	vendor/blip-buf/blip_buf.o)
BUILDDIR:=build

LDLIBS:=-lSDL2 -lSDL2_mixer -llua5.3

RM=rm -f
MKDIR=mkdir

PROJECT=amb-80
PLAYER=amb-player
TARGET=$(PROJECT)

define MKOBJDIR
$(1):
	@$(MKDIR) -p $(1)
endef

.PHONY: all clean cleanall

.DEFAULT_GOAL := all
all: $(OBJSUBDIRS) $(TARGET) $(PLAYER)

$(TARGET): $(OBJFILES)
	$(CC) $(LDFLAGS) $^ -o $(BUILDDIR)/$@ 

$(PLAYER): $(PLAYERFILES)
	$(CC) $(LDFLAGS) $^ -o $(BUILDDIR)/$@ 	

%.o: %.c
	$(CC) $(CCFLAGS) $(INCDIRS) $^ -c -o $@
