# $Id: Makefile,v 1.5 2002-06-10 22:01:23 jkf Exp $
ROOTDIR                 =$(N64_INST)

GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
MKSPRITEPATH = $(ROOTDIR)/bin/mksprite
HEADERPATH = $(ROOTDIR)/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
HEADERNAME = header

LIBS = -ldragon -lc -lm -ldragonsys -ln64memory
LINK_FLAGS = -G4 -L$(ROOTDIR)/lib -L$(ROOTDIR)/mips64-elf/lib $(LIBS) -Tn64ld.x
PROG_NAME = GENEMU
ASFLAGS = -mtune=vr4300 -march=vr4300

CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy

CFLAGS = -std=gnu99 -march=vr4300 -mtune=vr4300 -fno-strict-aliasing -Wall -G0 -Os -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include -Iobj -I.

OBJS = obj/z80.o

obj:
	mkdir obj

obj/z80.o: z80.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: obj/z80.o
