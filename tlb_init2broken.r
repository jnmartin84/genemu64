#  define LWHI	lwl		/* high part is left in big-endian	*/
#  define SWHI	swl		/* high part is left in big-endian	*/
#  define LWLO	lwr		/* low part is right in big-endian	*/
#  define SWLO	swr		/* low part is right in big-endian	*/

#define zero $0
#define at $1
#define v0 $2
#define v1 $3
#define a0 $4
#define a1 $5
#define a2 $6
#define a3 $7
#define t0 $8
#define t1 $9
#define t2 $10
#define t3 $11
#define t4 $12
#define t5 $13
#define t6 $14
#define t7 $15
#define s0 $16
#define s1 $17
#define s2 $18
#define s3 $19
#define s4 $20
#define s5 $21
#define s6 $22
#define s7 $23
#define t8 $24
#define t9 $25
#define k0 $26
#define k1 $27
#define gp $28
#define sp $29
#define fp $30
#define ra $31

	.global	TLB_INIT
	.set	nomips16
	.set	nomicromips
	.ent	TLB_INIT
	.type	TLB_INIT,	@function
	.set	noreorder
	.set	nomacro
	.align  3
TLB_INIT:
# /home/Jason/n64dev/n64inst/bin/mips64-elf-gcc -march=vr4300 -mtune=vr4300 -I/home/Jason/n64dev/n64inst/mips64-elf/include -c tlb_init.S -o obj/tlb_init.o
mtc0 $0,$2 # entry lo0
mtc0 $0,$3 # entry lo1
mtc0 $0,$5 # pagemask
# 0 for 4kb
# 0xf for 64kb (1111)
mtc0 $0,$6 # wired
mtc0 $0,$10 # entryhi

addiu $11,$0,31
clear_tlb_loop:
mtc0 $11, $0
nop
nop
nop
nop
nop
tlbwi
bne $11, $0, clear_tlb_loop
addi $11,-1


# now that the TLB is cleared/initialized

# add entries

# start with 2 TLB entries for m68k_ram
# first with pfn of m68k_ram >> 12
# second with pfn of (m68k_ram + 0x8000) >> 12

# vpn for first is 0x00e00000
# vpn for second is 0x00e08000

#entry lo
lw t0, %gp_rel(m68k_ram)(gp)
srl t3, t0, 12
sll t3, t3, 6
# 10 for C bits uncached 
# 1 for D bit, bit 2
# 1 for V bit, bit 1
# 1 for G bit, bit 0
ori t3, t3, 0x17

#addiu t1, zero, 0x00e0
#entry hi is 
# virtual page number >> 1
#sll t2, t1, 16
lui t2, 0x00e0

add t9,$0,$0
mtc0 t9, $0 # set index
mtc0 t3,$2 # entry lo0

#if 0
lw t0, %gp_rel(m68k_ram)(gp)
addiu t3, t0, 0x4000 
srl t3, t3, 12
sll t3, t3, 6
# 10 for C bits uncached 
# 1 for D bit, bit 2
# 1 for V bit, bit 1
# 1 for G bit, bit 0
ori t3, t3, 0x17
#endif
mtc0 t3,$3 # entry lo1
# 0x3 for 16kb (11)
addiu t4,$0,0x3
# gotta put the mask bits in the right place...
sll t4,t4,13
mtc0 t4,$5 # pagemask
mtc0 t2,$10 # entryhi

nop
nop
nop
nop
nop

tlbwi	

nop
nop
nop
nop
nop

#if 1
#entry lo
lw t0, %gp_rel(m68k_ram)(gp)
addiu t3, t0, 0x4000
addiu t3, t3, 0x4000
srl t3, t3, 12
sll t3, t3, 6
# 10 for C bits uncached 
# 1 for D bit, bit 2
# 1 for V bit, bit 1
# 1 for G bit, bit 0
ori t3, t3, 0x17

#addiu t1, zero, 0x00e0
lui t1, 0x00e0
addiu t2, t1, 0x4000
addiu t2, t2, 0x4000
#entry hi is 
# virtual page number >> 1
# move address over by 12 bits to get top 20
#srl t2, t1, 12
# move over one more bit to get 19
#srl t2, t2, 1
# move left 13 bits to make room for asid + 0 bit
#sll t2, t2, 13
# asid 0, do nothing else

addiu t9,$0,1
mtc0 t9, $0 # set index
mtc0 t3,$2 # entry lo0

#if 0
lw t0, %gp_rel(m68k_ram)(gp)
addiu t3, t0, 0x4000
addiu t3, t3, 0x4000
addiu t3, t3, 0x4000
srl t3, t3, 12
sll t3, t3, 6
# 10 for C bits uncached 
# 1 for D bit, bit 2
# 1 for V bit, bit 1
# 1 for G bit, bit 0
ori t3, t3, 0x17
#endif

mtc0 t3,$3 # entry lo1
# 0x3 for 16kb (11)
addiu t4,$0,0x3
# gotta put the mask bits in the right place...
sll t4,t4,13
mtc0 t4,$5 # pagemask
mtc0 t2,$10 # entryhi

nop
nop
nop
nop
nop

tlbwi	

nop
nop
nop
nop
nop
#endif

#if 0

# the full address, in t0
lw t0, %gp_rel(rom)(gp)
# entry lo is physical frame number
# move pfn into correct bits of entry reg
# address >> 12, << 6
srl t0, t0, 12
sll t0, t0, 6
# 0 for C bits cached 
# 0 for D bit, bit 2
# 1 for V bit, bit 1
# 1 for G bit, bit 0
ori t3, t0, 0x03

#entry hi is 
# virtual page number >> 1
# which in this case is 0
# and everything else is also 0 so just all 0
add t2,zero,zero
	

addiu t9,$0,1
mtc0 t9, $0 # set index
mtc0 t3,$2 # entry lo0
mtc0 t3,$3 # entry lo1

# 512kb for SONIC1 
# which would be...
# 2 256kb pages
# mask of 3f
addiu t4,$0,0x3f
# gotta put the mask bits in the right place...
sll t4,t4,13
mtc0 t4,$5 # pagemask

mtc0 t2,$10 # entryhi

nop
nop
nop
nop
nop

tlbwi	

nop
nop
nop
nop
nop
#endif

	
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	TLB_INIT
    .size   TLB_INIT, .-TLB_INIT
