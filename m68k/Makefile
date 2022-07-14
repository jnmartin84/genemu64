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

CFLAGS = -std=gnu99 -mno-shared -mno-abicalls -mno-branch-likely -mno-llsc -mno-check-zero-division -std=gnu99 -mabi=o64 -march=vr4300 -mtune=vr4300 -mips3 -Wall -G4 -O3 -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include -Iobj -I.
#-fno-strict-aliasing

OBJS = obj/m68kcpu.o obj/m68kopac.o obj/m68kopdm.o obj/m68kopnz.o obj/m68kops.o

all: m68k.o

m68k.o: obj $(OBJS)
#	$(LD) -r --oformat mips -o $@ $(OBJS)
	$(LD) -r -o $@ $(OBJS)

obj:
	mkdir obj

obj/m68kopac.o: obj/m68kopac.c obj/m68kops.h
	$(CC) $(CFLAGS) -c -o $@ $<

obj/m68kopdm.o: obj/m68kopdm.c obj/m68kops.h
	$(CC) $(CFLAGS) -c -o $@ $<

obj/m68kopnz.o: obj/m68kopnz.c obj/m68kops.h
	$(CC) $(CFLAGS) -c -o $@ $<

obj/m68kops.o: obj/m68kops.c obj/m68kops.h
	$(CC) $(CFLAGS) -c -o $@ $<

obj/m68kcpu.o: m68kcpu.c obj/m68kops.h
	$(CC) $(CFLAGS) -c -o $@ $<

#obj/m68kops.h: m68kmake
#	m68kmake obj m68k_in.c

#m68kmake: m68kmake.c m68k_in.c
#	$(CC) -g -O0 -Wall -o $@ $<

#clean:
#	/bin/rm -Rf obj m68kmake m68k.o

