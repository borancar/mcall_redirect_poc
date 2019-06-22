/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */
/* Modifications copyright (c) 2019 Boran Car <boran.car@gmail.com> */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h> // round()
#include <unistd.h>

#include <platform.h>

#define CMD_LINE_SIZE 32

uint64_t tee_get_csr(uint64_t csr)
{
    uint64_t value;
    asm volatile ("li a0, 1;" \
                  "mv a1, %[csr];" \
                  "mv a2, %[value];" \
                  "ecall"
                  :
                  : [csr] "r" (csr), [value] "r" (&value)
                  : "a0", "a1", "a2", "memory");
    return value;
}

// ------------------------------------------------------------------------
void print_cpu_info(void) {
// ------------------------------------------------------------------------
    // misa
    uint64_t misa = tee_get_csr(0x301);

    const int xlen = ((misa >> __riscv_xlen-2)&0b11)==1 ?  32 :
                     ((misa >> __riscv_xlen-2)&0b11)==2 ?  64 :
                     ((misa >> __riscv_xlen-2)&0b11)==1 ? 128 : 0;

    char misa_str[26+1]="";
    for (int i=0, j=0; i<26; i++)
        if ( (misa & (1ul << i)) !=0){
            misa_str[j++]=(char)('A'+i); misa_str[j]='\0';
        }

    printf("Machine ISA   : 0x%08x RV%d %s \n", (int)misa, xlen, misa_str);

    // mvendorid
    uint64_t mvendorid = tee_get_csr(0xF11);
    const char *mvendorid_str = (mvendorid==0x10e31913 ? "SiFive, Inc.\0" :
                                 mvendorid==0x489      ? "SiFive, Inc.\0" :
                                 mvendorid==0x57c      ? "Hex Five, Inc.\0" :
                                                         "\0");
    printf("Vendor        : 0x%08x %s \n", (int)mvendorid, mvendorid_str);

    // marchid
    uint64_t marchid = tee_get_csr(0xF12);
    const char *marchid_str = (mvendorid==0x489 && (int)misa==0x40101105    && marchid==0x80000002 ? "E21\0"  :
                               mvendorid==0x489 && (int)misa==0x40101105    && marchid==0x00000001 ? "E31\0"  :
                               mvendorid==0x489 && misa==0x8000000000101105 && marchid==0x00000001 ? "S51\0"  :
                               mvendorid==0x57c && (int)misa==0x40101105    && marchid==0x00000001 ? "X300\0" :
                               "\0");
    printf("Architecture  : 0x%08x %s \n", (int)marchid, marchid_str);

    // mimpid
    uint64_t mimpid = tee_get_csr(0xF13);
    printf("Implementation: 0x%08x \n", (int)mimpid );

    // mhartid
    uint64_t mhartid = tee_get_csr(0xF14);
    printf("Hart ID       : 0x%08x \n", (int)mhartid );

    // CPU Clock
    const int cpu_clk = round(CPU_FREQ/1E+6);
    printf("CPU clock     : %d MHz \n", cpu_clk );

}

// ------------------------------------------------------------------------
int cmpfunc(const void* a , const void* b){
// ------------------------------------------------------------------------
    const int ai = *(const int* )a;
    const int bi = *(const int* )b;
    return ai < bi ? -1 : ai > bi ? 1 : 0;
}

// ------------------------------------------------------------------------
void print_pmp(void){
// ------------------------------------------------------------------------

    #define TOR   0b00001000
    #define NA4   0b00010000
    #define NAPOT 0b00011000

    #define PMP_R 1<<0
    #define PMP_W 1<<1
    #define PMP_X 1<<2

    volatile uint64_t pmpcfg=0x0;
#if __riscv_xlen==32
    volatile uint32_t pmpcfg0 = tee_get_csr(0x3A0);
    volatile uint32_t pmpcfg1 = tee_get_csr(0x3A1);
    pmpcfg = pmpcfg1;
    pmpcfg <<= 32;
    pmpcfg |= pmpcfg0;
#else
    pmpcfg = tee_get_csr(0x3A0);
#endif

#if __riscv_xlen==32
    uint32_t pmpaddr[8];
#else
    uint64_t pmpaddr[8];
#endif
    for (int i=0; i<8; i++)
        pmpaddr[i] = tee_get_csr(0x3B0+i);

    for (int i=0; i<8; i++){

        const uint8_t cfg = (pmpcfg >> 8*i); if (cfg==0x0) continue;

        char rwx[3+1] = {cfg & PMP_R ? 'r':'-', cfg & PMP_W ? 'w':'-', cfg & PMP_X ? 'x':'-', '\0'};

        uint64_t start=0, end=0;

        char type[5+1]="";

        if ( (cfg & (TOR | NA4 | NAPOT)) == TOR){
            start = pmpaddr[i-1]<<2;
            end =  (pmpaddr[i]<<2) -1;
            strcpy(type, "TOR");

        } else if ( (cfg & (TOR | NA4 | NAPOT)) == NA4){
            start = pmpaddr[i]<<2;
            end =  start+4 -1;
            strcpy(type, "NA4");

        } else if ( (cfg & (TOR | NA4 | NAPOT)) == NAPOT){
            for (int j=0; j<__riscv_xlen; j++){
                if ( ((pmpaddr[i] >> j) & 0x1) == 0){
                    const uint64_t size = 1 << (3+j);
                    start = (pmpaddr[i] >>j) <<(j+2);
                    end = start + size -1;
                    strcpy(type, "NAPOT");
                    break;
                }
            }

        } else break;

#if __riscv_xlen==32
        printf("0x%08x 0x%08x %s %s \n", (unsigned int)start, (unsigned int)end, rwx, type);
#else
        printf("0x%016" PRIX64 " 0x%016" PRIX64 " %s %s \n", start, end, rwx, type);
#endif

    }

}

// ------------------------------------------------------------------------
int readline(char *cmd_line) {
// ------------------------------------------------------------------------
    int p=0;
    char c='\0';
    int esc=0;
    cmd_line[0] = '\0';
    static char history[CMD_LINE_SIZE+1]="";

    while(c!='\r'){

        if ( read(0, &c, 1) >0 ) {

            if (c=='\e'){
                esc=1;

            } else if (esc==1 && c=='['){
                esc=2;

            } else if (esc==2 && c=='3'){
                esc=3;

            } else if (esc==3 && c=='~'){ // del key
                for (int i=p; i<strlen(cmd_line); i++) cmd_line[i]=cmd_line[i+1];
                write(1, "\e7", 2); // save curs pos
                write(1, "\e[K", 3); // clear line from curs pos
                write(1, &cmd_line[p], strlen(cmd_line)-p);
                write(1, "\e8", 2); // restore curs pos
                esc=0;

            } else if (esc==2 && c=='C'){ // right arrow
                esc=0;
                if (p < strlen(cmd_line)){
                    p++;
                    write(1, "\e[C", 3);
                }

            } else if (esc==2 && c=='D'){ // left arrow
                esc=0;
                if (p>0){
                    p--;
                    write(1, "\e[D", 3);
                }

            } else if (esc==2 && c=='A'){ // up arrow
                esc=0;
                if (strlen(history)>0){
                    p=strlen(history);
                    strcpy(cmd_line, history);
                    write(1, "\e[2K", 4); // 2K clear entire line - cur pos dosn't change
                    write(1, "\rZ1 > ", 6);
                    write(1, &cmd_line[0], strlen(cmd_line));
                }

            } else if (esc==2 && c=='B'){ // down arrow
                esc=0;

            } else if ((c=='\b' || c=='\x7f') && p>0 && esc==0){ // backspace
                p--;
                for (int i=p; i<strlen(cmd_line); i++) cmd_line[i]=cmd_line[i+1];
                write(1, "\e[D", 3);
                write(1, "\e7", 2);
                write(1, "\e[K", 3);
                write(1, &cmd_line[p], strlen(cmd_line)-p);
                write(1, "\e8", 2);

            } else if (c>=' ' && c<='~' && p < CMD_LINE_SIZE && esc==0){
                for (int i = CMD_LINE_SIZE-1; i > p; i--) cmd_line[i]=cmd_line[i-1]; // make room for 1 ch
                cmd_line[p]=c;
                write(1, "\e7", 2); // save curs pos
                write(1, "\e[K", 3); // clear line from curs pos
                write(1, &cmd_line[p], strlen(cmd_line)-p); p++;
                write(1, "\e8", 2); // restore curs pos
                write(1, "\e[C", 3); // move curs right 1 pos

            } else
                esc=0;
        }

    }

    for (int i = CMD_LINE_SIZE-1; i > 0; i--)
        if (cmd_line[i]==' ') cmd_line[i]='\0'; else break;

    if (strlen(cmd_line)>0)
        strcpy(history, cmd_line);

    return strlen(cmd_line);

}

void cli (void) {
    char cmd_line[CMD_LINE_SIZE+1]="";

    while (1) {
        write(1, "\n\rZ1 > ", 7);

        readline(cmd_line);

        write(1, "\n", 1);

    char * tk1 = strtok (cmd_line, " ");
    char * tk2 = strtok (NULL, " ");
    char * tk3 = strtok (NULL, " ");

        if (tk1 != NULL && strcmp(tk1, "load")==0){
            if (tk2 != NULL){
                uint8_t data = 0x00;
                const uint64_t addr = strtoull(tk2, NULL, 16);
                asm ("lbu %0, (%1)" : "+r"(data) : "r"(addr));
                printf("0x%08x : 0x%02x \n", (unsigned int)addr, data);
            } else printf("Syntax: load address \n");

        } else if (tk1 != NULL && strcmp(tk1, "store")==0){
            if (tk2 != NULL && tk3 != NULL){
                const uint32_t data = (uint32_t)strtoul(tk3, NULL, 16);
                const uint64_t addr = strtoull(tk2, NULL, 16);

                if ( strlen(tk3) <=2 )
                    asm ( "sb %0, (%1)" : : "r"(data), "r"(addr));
                else if ( strlen(tk3) <=4 )
                    asm ( "sh %0, (%1)" : : "r"(data), "r"(addr));
                else
                    asm ( "sw %0, (%1)" : : "r"(data), "r"(addr));

                printf("0x%08x : 0x%02x \n", (unsigned int)addr, (unsigned int)data);
            } else printf("Syntax: store address data \n");

        } else if (tk1 != NULL && strcmp(tk1, "exec")==0){
            if (tk2 != NULL){
                const uint64_t addr = strtoull(tk2, NULL, 16);
                asm ( "jr (%0)" : : "r"(addr));
            } else printf("Syntax: exec address \n");

        } else if (tk1 != NULL && strcmp(tk1, "restart")==0){
            asm ("j _start");

        } else if (tk1 != NULL && strcmp(tk1, "pmp")==0){
            print_pmp();

        } else
            printf("Commands: load store exec pmp restart \n");

    }

}

void trap(uintptr_t *regs, uintptr_t scause, uintptr_t sepc)
{
    switch (scause) {
        case 4:
            printf("Load address misaligned! epc=0x%016x\n", sepc);
            break;
        case 5:
            printf("Load access fault! epc=0x%016x\n", sepc);
            break;
        case 6:
            printf("Store/AMO address misaligned! epc=0x%016x\n", sepc);
            break;
        case 7:
            printf("Store/AMO access fault! epc=0x%016x\n", sepc);
            break;
    }

    asm volatile ("csrr t0, sepc;" \
                  "addi t0, t0, 4;" \
                  "csrw sepc, t0;"
                  : : : "t0");
}

// ------------------------------------------------------------------------
int main (void) {
// ------------------------------------------------------------------------
    open("UART", 0, 0);

                 printf("\e[2J\e[H"); // clear terminal screen
    printf("===========================================================================\n");
    printf("                              Terminal                                     \n");
    printf("       Copyright (C) 2018 Hex Five Security Inc. All Rights Reserved       \n");
    printf("===========================================================================\n");
    print_cpu_info();

    cli();
}
