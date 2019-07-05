// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Core/HW/MemoryInterface.h"

#include "Common/ChunkFile.h"
#include "Common/CommonTypes.h"
#include "Core/HW/MMIO.h"

namespace MemoryInterface
{
// internal hardware addresses
enum
{
  MI_REGION0_FIRST = 0x000,
  MI_REGION0_LAST = 0x002,
  MI_REGION1_FIRST = 0x004,
  MI_REGION1_LAST = 0x006,
  MI_REGION2_FIRST = 0x008,
  MI_REGION2_LAST = 0x00A,
  MI_REGION3_FIRST = 0x00C,
  MI_REGION3_LAST = 0x00E,
  MI_PROT_TYPE = 0x010,
  MI_IRQMASK = 0x01C,
  MI_IRQFLAG = 0x01E,
  MI_UNKNOWN1 = 0x020,
  MI_PROT_ADDR_LO = 0x022,
  MI_PROT_ADDR_HI = 0x024,
  MI_TIMER0_HI = 0x032,
  MI_TIMER0_LO = 0x034,
  MI_TIMER1_HI = 0x036,
  MI_TIMER1_LO = 0x038,
  MI_TIMER2_HI = 0x03A,
  MI_TIMER2_LO = 0x03C,
  MI_TIMER3_HI = 0x03E,
  MI_TIMER3_LO = 0x040,
  MI_TIMER4_HI = 0x042,
  MI_TIMER4_LO = 0x044,
  MI_TIMER5_HI = 0x046,
  MI_TIMER5_LO = 0x048,
  MI_TIMER6_HI = 0x04A,
  MI_TIMER6_LO = 0x04C,
  MI_TIMER7_HI = 0x04E,
  MI_TIMER7_LO = 0x050,
  MI_TIMER8_HI = 0x052,
  MI_TIMER8_LO = 0x054,
  MI_TIMER9_HI = 0x056,
  MI_TIMER9_LO = 0x058,
  MI_UNKNOWN2 = 0x05A,
};

union MIRegion
{
  u32 hex;
  struct
  {
    u16 first_page;
    u16 last_page;
  };
};

union MIProtType
{
  u16 hex;
  struct
  {
    u16 reg0 : 2;
    u16 reg1 : 2;
    u16 reg2 : 2;
    u16 reg3 : 2;
    u16 : 8;
  };
};

union MIIRQMask
{
  u16 hex;
  struct
  {
    u16 reg0 : 1;
    u16 reg1 : 1;
    u16 reg2 : 1;
    u16 reg3 : 1;
    u16 all_regs : 1;
    u16 : 11;
  };
};

union MIIRQFlag
{
  u16 hex;
  struct
  {
    u16 reg0 : 1;
    u16 reg1 : 1;
    u16 reg2 : 1;
    u16 reg3 : 1;
    u16 all_regs : 1;
    u16 : 11;
  };
};

union MIProtAddr
{
  u32 hex;
  struct
  {
    u16 lo;
    u16 hi;
  };
  struct
  {
    u32 : 5;
    u32 addr : 25;
    u32 : 2;
  };
};

union MITimer
{
  u32 hex;
  struct
  {
    u16 lo;
    u16 hi;
  };
};

struct MIMemStruct
{
  MIRegion regions[4];
  MIProtType prot_type;
  MIIRQMask irq_mask;
  MIIRQFlag irq_flag;
  u16 unknown1;
  MIProtAddr prot_addr;
  MITimer timers[10];
  u16 unknown2;
};

// STATE_TO_SAVE
static MIMemStruct g_mi_mem;

void DoState(PointerWrap& p)
{
  p.Do(g_mi_mem);
}

void RegisterMMIO(MMIO::Mapping* mmio, u32 base)
{
  for (int i = MI_REGION0_FIRST; i <= MI_REGION3_LAST; i += 4)
  {
    auto& region = g_mi_mem.regions[i / 4];
    mmio->Register(base | i, MMIO::DirectRead<u16>(&region.first_page),
                   MMIO::DirectWrite<u16>(&region.first_page));
    mmio->Register(base | (i + 2), MMIO::DirectRead<u16>(&region.last_page),
                   MMIO::DirectWrite<u16>(&region.last_page));
  }

  mmio->Register(base | MI_PROT_TYPE, MMIO::DirectRead<u16>(&g_mi_mem.prot_type.hex),
                 MMIO::DirectWrite<u16>(&g_mi_mem.prot_type.hex));

  mmio->Register(base | MI_IRQMASK, MMIO::DirectRead<u16>(&g_mi_mem.irq_mask.hex),
                 MMIO::DirectWrite<u16>(&g_mi_mem.irq_mask.hex));

  mmio->Register(base | MI_IRQFLAG, MMIO::DirectRead<u16>(&g_mi_mem.irq_flag.hex),
                 MMIO::DirectWrite<u16>(&g_mi_mem.irq_flag.hex));

  mmio->Register(base | MI_UNKNOWN1, MMIO::DirectRead<u16>(&g_mi_mem.unknown1),
                 MMIO::DirectWrite<u16>(&g_mi_mem.unknown1));

  // The naming is confusing here: the registed contains the lower part of
  // the address (hence MI_..._LO but this is still the high part of the
  // overall register.
  mmio->Register(base | MI_PROT_ADDR_LO, MMIO::DirectRead<u16>(&g_mi_mem.prot_addr.hi),
                 MMIO::DirectWrite<u16>(&g_mi_mem.prot_addr.hi));
  mmio->Register(base | MI_PROT_ADDR_HI, MMIO::DirectRead<u16>(&g_mi_mem.prot_addr.lo),
                 MMIO::DirectWrite<u16>(&g_mi_mem.prot_addr.lo));

  for (int i = 0; i < 10; ++i)
  {
    auto& timer = g_mi_mem.timers[i];
    mmio->Register(base | (MI_TIMER0_HI + 4 * i), MMIO::DirectRead<u16>(&timer.hi),
                   MMIO::DirectWrite<u16>(&timer.hi));
    mmio->Register(base | (MI_TIMER0_LO + 4 * i), MMIO::DirectRead<u16>(&timer.lo),
                   MMIO::DirectWrite<u16>(&timer.lo));
  }

  mmio->Register(base | MI_UNKNOWN2, MMIO::DirectRead<u16>(&g_mi_mem.unknown2),
                 MMIO::DirectWrite<u16>(&g_mi_mem.unknown2));

  for (int i = 0; i < 0x1000; i += 4)
  {
    mmio->Register(base | i, MMIO::ReadToSmaller<u32>(mmio, base | i, base | (i + 2)),
                   MMIO::WriteToSmaller<u32>(mmio, base | i, base | (i + 2)));
  }
}

}  // end of namespace MemoryInterface
