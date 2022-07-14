ROOTDIR=$(N64_INST)

GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
MKSPRITEPATH = $(ROOTDIR)/bin/mksprite
HEADERPATH = $(ROOTDIR)/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
HEADERNAME = header

LIBS = -ldragon -lc -lm -ldragonsys
LINK_FLAGS = -G4 -L$(ROOTDIR)/mips64-elf/lib -L$(ROOTDIR)/lib $(LIBS) -Tn64.ld
PROG_NAME = GENEMU
#CFLAGS = -std=gnu99 -march=vr4300 -mtune=vr4300 -fno-strict-aliasing -mmemcpy -mhard-float -mno-check-zero-division -Wall -Werror -fno-strict-aliasing -G4 -O2 -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
CFLAGS = -mno-shared -mno-abicalls -mno-branch-likely -mno-llsc -mno-check-zero-division -std=gnu99 -mabi=o64 -march=vr4300 -mtune=vr4300 -mips3  -G4 -O3 -I$(ROOTDIR)/mips64-elf/includ 
#CFLAGS_ASM = -std=gnu99 -march=vr4300 -mtune=vr4300 -fno-strict-aliasing -Wall -G4 -O2 -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
ASFLAGS = -mtune=vr4300 -march=vr4300

CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy
OBJDUMP = $(GCCN64PREFIX)objdump

O=obj

##OBJS = $(O)/main.o $(O)/loader.o $(O)/input.o $(O)/SndNintendo64.o $(O)/SN76489.o $(O)/Sound.o $(O)/m68k.o $(O)/z80.o $(O)/vdp.o $(O)/misc.o
##OBJS += $(O)/md5c.o $(O)/m68k/m68k.o $(O)/z80/z80.o $(O)/video.o $(O)/init.o

#OBJS = $(O)/main.o $(O)/loader.o $(O)/input.o $(O)/m68k.o $(O)/z80.o $(O)/vdp.o $(O)/misc.o
OBJS = $(O)/tlb_init2.o $(O)/rsp.o $(O)/math.o $(O)/nemesis.o $(O)/memory_stream.o $(O)/kosinski_decompress.o $(O)/main.o $(O)/loader.o $(O)/input.o $(O)/mem68k.o $(O)/z80.o $(O)/vdp.o
OBJS += $(O)/md5c.o $(O)/m68k/m68k.o $(O)/z80/z80.o $(O)/video.o $(O)/init.o $(O)/n64_memcpy.o $(O)/n64_memset.o $(O)/exception.o
 
$(PROG_NAME).z64: $(PROG_NAME).elf rom.dfs
	$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	rm -f $(PROG_NAME).z64
	$(N64TOOL) -l 8M -t "GENEMU"				\
		-h $(HEADERPATH)/$(HEADERNAME)			\
		-o $(PROG_NAME).z64 $(PROG_NAME).bin		\
		-s 1M rom.dfs
	$(CHKSUM64PATH) $(PROG_NAME).z64

$(PROG_NAME).v64: $(PROG_NAME).elf rom.dfs
	$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	rm -f $(PROG_NAME).v64
	$(N64TOOL) -b -l 8M -t "GENEMU"				\
		-h $(HEADERPATH)/$(HEADERNAME)			\
		-o $(PROG_NAME).v64 $(PROG_NAME).bin		\
		-s 1M rom.dfs
	$(CHKSUM64PATH) $(PROG_NAME).v64

$(PROG_NAME).elf : $(OBJS)
	$(LD) -o $(PROG_NAME).elf $(OBJS) $(LINK_FLAGS)
	$(OBJDUMP) -t $(PROG_NAME).elf | grep -v 'm68k' > $(PROG_NAME)_symbols.txt


load: $(PROG_NAME).z64
	./load_genemu.sh

copy: $(PROG_NAME).z64
	cp $(PROG_NAME).z64 ~/

all: $(PROG_NAME).z64

clean:
	rm *.z64 *.elf *.bin *.dfs $(PROG_NAME)_symbols.txt
	rm -f $(OBJS)

$(S)/%.s:	%.c
	$(CC) $(CFLAGS_ASM) -S $< -o $@

$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

rom.dfs:
	$(MKDFSPATH) rom.dfs ./filesystem/

#############################################################
#
#############################################################
