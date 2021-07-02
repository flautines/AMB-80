# SOURCE AND OBJECT CONFIGURATION
CC:=clang
CCFLAGS:=-g -Wall -pedantic -std=c11
LUADIR:=-I/usr/include/lua5.3
INCDIRS:=-Iinclude -Isrc -Ivendor/blip-buf $(shell sdl2-config --cflags) $(LUADIR)
LDFLAGS:=$(shell sdl2-config --libs)
SUBDIRS:=src/api src/core src/ext src/studio src/system/sdl src/
OBJSUBDIRS:=$(foreach DIR, $(SUBDIRS), $(patsubst $(SRCDIR)%, $(OBJDIR)%, $(DIR)))
SRCFILES:=$(foreach DIR, $(SUBDIRS), $(wildcard $(DIR)/*.c))
OBJFILES:=$(patsubst $(SRCDIR)%, $(OBJDIR)%, $(SRCFILES:%.c=%.o)) \
	vendor/blip-buf/blip_buf.o

LDLIBS:=-lSDL2 -lSDL2_mixer -llua5.3

RM=rm -f
MKDIR=mkdir

PROJECT=amb-80
TARGET=$(PROJECT)

define MKOBJDIR
$(1):
	@$(MKDIR) -p $(1)
endef

.PHONY: all clean cleanall

.DEFAULT_GOAL := all
all: $(OBJSUBDIRS) $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(LDFLAGS) $^ -o $@ 

%.o: %.c
	$(CC) $(CCFLAGS) $(INCDIRS) $^ -c -o $@
