// byte *param_1,void *param_2)
// A0 == source
// A1 == dest

void Nem_Build_Code_Table()
{
  uint32_t param_1 = REG_A0;
  uint32_t param_2 = REG_A1;
  
  byte bVar1;

  short sVar2;
  short sVar4;

  ushort uVar3;
  ushort uVar5;
  
  byte *pbVar6;
  byte *pbVar7;
  
  uVar3 = (ushort)m68k_read_memory_16(param_1);
  //*param_1;
  
  pbVar6 = param_1 + 1;
  
  
  
  while (pbVar7 = pbVar6, uVar5 = uVar3, (char)uVar3 != -1) {
    while( true ) {

	pbVar6 = pbVar7 + 1;
      bVar1 = m68k_read_memory_8(pbVar7); //*pbVar7;

      uVar3 = (ushort)bVar1;
      if (0x7f < bVar1) break;
      uVar5 = uVar5 & 0xf | (ushort)bVar1 & 0x70 | (uVar3 & 0xf) << 8;
      uVar3 = 8 - (uVar3 & 0xf);
      if (uVar3 == 0) {
        pbVar7 = pbVar7 + 2;
        
		
		*(ushort *)((int)param_2 + (int)(short)((ushort)*pbVar6 * 2)) = uVar5;
      }
      else {
        pbVar7 = pbVar7 + 2;
        sVar2 = ((ushort)*pbVar6 << ((uint)uVar3 & 0x3f)) * 2;
        sVar4 = (1 << ((uint)uVar3 & 0x3f)) + -1;
        do {
//          *(ushort *)((int)param_2 + (int)sVar2) = uVar5;
        
m68k_write_memory_16((int)param_2 + (int)sVar2, uVar5);

		sVar2 += 2;
          sVar4 += -1;
        } while (sVar4 != -1);
      }
    }
  }
  return;
}

