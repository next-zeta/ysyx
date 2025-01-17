/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

void display_memory_read(paddr_t addr, int len);
void display_memory_write(paddr_t addr, int len);

#if   defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
static uint8_t sram[SRAM_SIZE] PG_ALIGN = {};
#endif

uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + MBASE; }
uint8_t* sram_to_host(paddr_t paddr) { return sram + paddr - SRAM_BASE; }
paddr_t host_to_sram(uint8_t *haddr) { return haddr - sram + SRAM_BASE; }

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret;
  if(addr >= MBASE && addr <= MBASE + MSIZE){
    ret = host_read(guest_to_host(addr), len);
  }
  else {
    ret = host_read(sram_to_host(addr), len);
  }
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  if(addr >= MBASE && addr <= MBASE + MSIZE){
    host_write(guest_to_host(addr), len, data);
  }
  else {
    host_write(sram_to_host(addr), len, data);
  }
}

static void out_of_bound(paddr_t addr) {
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
}

void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif
  IFDEF(CONFIG_MEM_RANDOM, memset(pmem, rand(), MSIZE));
  IFDEF(CONFIG_YSYXSOC_MEM,memset(sram, 0, SRAM_SIZE));
  uint8_t *sram = (uint8_t *)malloc(SRAM_SIZE);
  memset(sram, 0, SRAM_SIZE);

  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}

word_t paddr_read(paddr_t addr, int len) {
  IFDEF(CONFIG_MTRACE, display_memory_read(addr, len));
  if (likely(in_pmem(addr))) return pmem_read(addr, len);
  IFDEF(CONFIG_DEVICE, return mmio_read(addr, len));
  out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
  IFDEF(CONFIG_MTRACE, display_memory_write(addr, len));
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
  IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return);
  out_of_bound(addr);
}
