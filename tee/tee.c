// Copyright (c) 2013, The Regents of the University of California (Regents).
// All Rights Reserved
// See LICENSE.SiFive for license details.
// Modifications copyright (c) Boran Car <boran.car@gmail.com>

#include <stdint.h>
#include <string.h>

#include "bits.h"
#include "encoding.h"

void tee_provide_csr(uint64_t csr, uintptr_t dst)
{
    uint64_t *dstp = (uint64_t*)dst;
    switch (csr) {
        case 0xF11:
            *dstp = read_csr(mvendorid);
            break;
        case 0xF12:
            *dstp = read_csr(marchid);
            break;
        case 0xF13:
            *dstp = read_csr(mimpid);
            break;
        case 0xF14:
            *dstp = read_csr(mhartid);
            break;
        case 0x301:
            *dstp = read_csr(misa);
            break;
        case 0x3A0:
            *dstp = read_csr(pmpcfg0);
            break;
        case 0x3A1:
            *dstp = read_csr(pmpcfg1);
            break;
        case 0x3A2:
            *dstp = read_csr(pmpcfg2);
            break;
        case 0x3A3:
            *dstp = read_csr(pmpcfg3);
            break;
        case 0x3B0:
            *dstp = read_csr(pmpaddr0);
            break;
        case 0x3B1:
            *dstp = read_csr(pmpaddr1);
            break;
        case 0x3B2:
            *dstp = read_csr(pmpaddr2);
            break;
        case 0x3B3:
            *dstp = read_csr(pmpaddr3);
            break;
        case 0x3A4:
            *dstp = read_csr(pmpaddr4);
            break;
        case 0x3A5:
            *dstp = read_csr(pmpaddr5);
            break;
        case 0x3A6:
            *dstp = read_csr(pmpaddr6);
            break;
        case 0x3A7:
            *dstp = read_csr(pmpaddr7);
            break;
        default:
            *dstp = 0;
    }
}

uint8_t key[16] __attribute__((section(".otp"))) = {0xde, 0xad, 0xbe, 0xef, 0xba, 0xdc, 0x0d, 0xe0, 0xde, 0xad, 0xc0, 0xde, 0xc0, 0xde, 0xde, 0xad};

void mcall_trap(uintptr_t *regs, uintptr_t mcause, uintptr_t mepc)
{
    write_csr(mepc, mepc + 4);
    uintptr_t cmd = regs[10];

    switch(cmd) {
        case 1:
            tee_provide_csr(regs[11], regs[12]);
            break;
    }
}

void redirect_trap(uintptr_t *regs, uintptr_t mcause, uintptr_t mepc)
{
    write_csr(sbadaddr, read_csr(mbadaddr));
    write_csr(sepc, mepc);
    write_csr(scause, mcause);
    write_csr(mepc, read_csr(stvec));

    uintptr_t mstatus = read_csr(mstatus);
    uintptr_t new_mstatus = mstatus & ~(MSTATUS_SPP | MSTATUS_SPIE | MSTATUS_SIE);
    uintptr_t mpp_s = MSTATUS_MPP & (MSTATUS_MPP >> 1);
    new_mstatus |= (mstatus * (MSTATUS_SPIE / MSTATUS_SIE)) & MSTATUS_SPIE;
    new_mstatus |= (mstatus / (mpp_s / MSTATUS_SPP)) & MSTATUS_SPP;
    new_mstatus |= mpp_s;
    write_csr(mstatus, new_mstatus);

    extern void __redirect_trap();
    return __redirect_trap();
}

extern uint8_t _sp;

int main(int argc, char *argv[])
{
    write_csr(satp, 0);
    uintptr_t pmpc = ((PMP_NAPOT | PMP_R | PMP_W) << 16)
                     | ((PMP_NAPOT | PMP_R | PMP_W | PMP_X) << 8)
                     | (PMP_NAPOT | PMP_R | PMP_W | PMP_X);
    asm volatile ("csrw pmpaddr0, %1;" \
                  "csrw pmpaddr1, %2;" \
                  "csrw pmpaddr2, %3;" \
                  "csrw pmpcfg0,  %0;"
                  : : "r"(pmpc),
                      "r"((0x20010000UL >> 2) | (((1<<(16-1))-1)>>2)),
                      "r"(( 0x8001000UL >> 2) | (((1<<(12-1))-1)>>2)),
                      "r"((0x10010000UL >> 2) | (((1<<( 8-1))-1)>>2)));

    uintptr_t mstatus = read_csr(mstatus);
    mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_S);
    mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 0);
    write_csr(mstatus, mstatus);
    write_csr(mscratch, &_sp - 32*REGBYTES);
    write_csr(mepc, 0x20010000);
    asm volatile ("mret");

    __builtin_unreachable();
}
