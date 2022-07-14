// RDP Status Read Flags:
constant RDP_XBS=$001 // DPC_STATUS: Use XBUS DMEM DMA Or DRAM DMA =Bit 0
constant RDP_FRZ=$002 // DPC_STATUS: RDP Frozen =Bit 1
constant RDP_FLS=$004 // DPC_STATUS: RDP Flushed =Bit 2
constant RDP_GCL=$008 // DPC_STATUS: GCLK Alive =Bit 3
constant RDP_TMB=$010 // DPC_STATUS: TMEM Busy =Bit 4
constant RDP_PLB=$020 // DPC_STATUS: RDP PIPELINE Busy =Bit 5
constant RDP_CMB=$040 // DPC_STATUS: RDP COMMAND Unit Busy =Bit 6
constant RDP_CMR=$080 // DPC_STATUS: RDP COMMAND Buffer Ready =Bit 7
constant RDP_DMA=$100 // DPC_STATUS: RDP DMA Busy =Bit 8
constant RDP_CME=$200 // DPC_STATUS: RDP COMMAND END Register Valid =Bit 9
constant RDP_CMS=$400 // DPC_STATUS: RDP COMMAND START Register Valid =Bit 10

// RDP Status Write Flags:
constant CLR_XBS=$001 // DPC_STATUS: Clear XBUS DMEM DMA =Bit 0
constant SET_XBS=$002 // DPC_STATUS:   Set XBUS DMEM DMA =Bit 1
constant CLR_FRZ=$004 // DPC_STATUS: Clear FREEZE =Bit 2
constant SET_FRZ=$008 // DPC_STATUS:   Set FREEZE =Bit 3
constant CLR_FLS=$010 // DPC_STATUS: Clear FLUSH =Bit 4
constant SET_FLS=$020 // DPC_STATUS:   Set FLUSH =Bit 5
constant CLR_TMC=$040 // DPC_STATUS: Clear TMEM COUNTER =Bit 6
constant CLR_PLC=$080 // DPC_STATUS: Clear PIPELINE COUNTER =Bit 7
constant CLR_CMC=$100 // DPC_STATUS: Clear COMMAND COUNTER =Bit 8
constant CLR_CLK=$200 // DPC_STATUS: Clear CLOCK COUNTER =Bit 9

// RSP Side DP
macro RSPDPC(start, end) { // Run DPC Command Buffer: Start Address, End Address
  la a0,{start} // A0 = DPC Command Start Address
  mtc0 a0,c8 // Store DPC Command Start Address To DP Start Register ($A4100000)
  addi a0,{end}-{start} // A0 = DPC Command End Address
  mtc0 a0,c9 // Store DPC Command End Address To DP End Register ($A4100004)
}

macro RSPDPStatusWR(flags) { // RDP Status: Write Flags
  ori t0,r0,{flags} // T0 = DP Status: Write Flags
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPClearXBUS() { // RDP Status: Clear XBUS (Switch To CPU RDRAM For RDP Commands)
  ori t0,r0,CLR_XBS // T0 = DP Status: Clear XBUS DMEM DMA (Bit 0)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPSetXBUS() { // RDP Status: Set XBUS (Switch To RSP DMEM For RDP Commands)
  ori t0,r0,SET_XBS // T0 = DP Status: Set XBUS DMEM DMA (Bit 1)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPClearFREEZE() { // RDP Status: Clear FREEZE
  ori t0,r0,CLR_FRZ // T0 = DP Status: Clear FREEZE (Bit 2)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPSetFREEZE() { // RDP Status: Set FREEZE
  ori t0,r0,SET_FRZ // T0 = DP Status: Set FREEZE (Bit 3)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPClearFLUSH() { // RDP Status: Clear FLUSH
  ori t0,r0,CLR_FLS // T0 = DP Status: Clear FLUSH (Bit 4)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPSetFLUSH() { // RDP Status: Set FLUSH
  ori t0,r0,SET_FLS // T0 = DP Status: Set FLUSH (Bit 5)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPClearTMEM() { // RDP Status: Clear TMEM COUNTER
  ori t0,r0,CLR_TMC // T0 = DP Status: Clear TMEM COUNTER (Bit 6)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPClearPIPE() { // RDP Status: Clear PIPELINE COUNTER
  ori t0,r0,CLR_PLC // T0 = DP Status: Clear PIPELINE COUNTER (Bit 7)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPClearCOMMAND() { // RDP Status: Clear COMMAND COUNTER
  ori t0,r0,CLR_CMC // T0 = DP Status: Clear COMMAND COUNTER (Bit 8)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPClearCLOCK() { // RDP Status: Clear CLOCK COUNTER
  ori t0,r0,CLR_CLK // T0 = DP Status: Clear CLOCK COUNTER (Bit 9)
  mtc0 t0,c11 // Store DP Status To DP Status Register ($A410000C)
}

macro RSPDPCOMMANDBusy() { // Wait For RDP Command Buffer BUSY To Finish (DMA Busy = 0)
  -
    mfc0 t0,c13 // T0 = RDP Command Buffer BUSY Register ($A4100014)
    bnez t0,- // IF TRUE RDP Command Buffer Busy
    nop // Delay Slot
}

macro RSPDPPIPEBusy() { // Wait For RDP Pipe BUSY To Finish (DMA Busy = 0)
  -
    mfc0 t0,c14 // T0 = RDP Pipe BUSY Register ($A4100018)
    bnez t0,- // IF TRUE RDP Pipe Busy
    nop // Delay Slot
}

macro RSPDPTMEMBusy() { // Wait For RDP TMEM BUSY To Finish (DMA Busy = 0)
  -
    mfc0 t0,c15 // T0 = RDP TMEM BUSY Register ($A410001C)
    bnez t0,- // IF TRUE RDP TMEM Busy
    nop // Delay Slot
}