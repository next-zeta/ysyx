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

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>
#include <isa.h>

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])
#define csr(imm) *csr_return(imm)
#define ECALL(pc) ecall(pc)

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

static inline word_t *csr_return(word_t imm){
  switch (imm)
  {
  case 0x305:
    return &cpu.csr.mtvec;
  case 0x341:
    return &cpu.csr.mepc;
  case 0x300:
    return &cpu.csr.mstatus;
  case 0x342:
    return &cpu.csr.mcause;
  default:
    assert(0);
    break;
  }
}

static inline word_t ecall(vaddr_t pc){
  bool success = true;
  vaddr_t dnpc;
  word_t NO = isa_reg_str2val("$a7",&success);
  dnpc = isa_raise_intr(NO,pc);
  return dnpc;
}

#endif
