#!/bin/sh
cd /home/jason/n64dev/genemu64
rm ./obj/rsp.o
rm ./rsp_code.h
#rm ./last_dma.h
cd ./bass
./bass.exe ./combiner.asm
#./bass.exe ./last_dma.asm
xxd --include rsp_code > ./../rsp_code.h
#xxd --include last_dma > ./../last_dma.h
cd ./..
make
