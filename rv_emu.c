#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rv_emu.h"
#include "bits.h"

#define DEBUG 0

static void unsupported(char *s, uint32_t n) {
    printf("unsupported %s 0x%x\n", s, n);
    exit(-1);
}

void rv_init(struct rv_state_st *rsp, uint32_t *func,
             uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    int i;

    // Zero out registers
    for (i = 0; i < RV_NREGS; i += 1) {
        rsp->regs[i] = 0;
    }

    // Zero out the stack
    for (i = 0; i < STACK_SIZE; i += 1) {
        rsp->stack[i] = 0;
    }

    // Initialize the Program Counter
    rsp->pc = (uint64_t) func;

    // Register 0 (zero) is always 0
    rsp->regs[RV_ZERO] = 0;

    // Initialize the Link Register to a sentinel value
    rsp->regs[RV_RA] = 0;

    // Initialize Stack Pointer to the logical bottom of the stack
    rsp->regs[RV_SP] = (uint64_t) &rsp->stack[STACK_SIZE];

    // Initialize the first 4 arguments in emulated r0-r3
    rsp->regs[RV_A0] = a0;
    rsp->regs[RV_A1] = a1;
    rsp->regs[RV_A2] = a2;
    rsp->regs[RV_A3] = a3;


    memset(&rsp->analysis, 0, sizeof(struct analysis_st));
    cache_init(&rsp->i_cache);
}

uint32_t rv_get_rd (uint32_t iw) {
    return get_bits(iw, 7, 5);
}

uint32_t rv_get_rs1 (uint32_t iw) {
    return get_bits(iw, 15, 5);
}

uint32_t rv_get_rs2 (uint32_t iw) {
    return get_bits(iw, 20, 5);
}

uint32_t rv_get_funct3 (uint32_t iw) {
    return get_bits(iw, 12, 3);
}

uint32_t rv_get_funct7 (uint32_t iw) {
    return get_bits(iw, 25, 7);
}

void emu_r_type(struct rv_state_st *rsp, uint32_t iw) {
    uint32_t rd = rv_get_rd(iw);
    uint32_t rs1 = rv_get_rs1(iw);
    uint32_t rs2 = rv_get_rs2(iw);
    uint32_t funct3 = rv_get_funct3(iw);
    uint32_t funct7 = rv_get_funct7(iw);

    if (funct3 == 0b000 && funct7 == 0b0000000) {
        rsp->regs[rd] = rsp->regs[rs1] + rsp->regs[rs2];
    } else if (funct3 == 0b000 && funct7 == 0b0000001) {
        rsp->regs[rd] = rsp->regs[rs1] * rsp->regs[rs2];
    } else if (funct3 == 0b000 && funct7 == 0b0100000){
        rsp->regs[rd] = rsp->regs[rs1] - rsp->regs[rs2];
    } else if (funct3 == 0b001 && funct7 == 0b0000000) {
        rsp->regs[rd] = rsp->regs[rs1] << rsp->regs[rs2];
    } else if (funct3 == 0b101 && funct7 == 0b0000000) {
        rsp->regs[rd] = rsp->regs[rs1] >> rsp->regs[rs2];
    } else if (funct3 == 0b111 && funct7 ==0b0000000) {
        rsp->regs[rd] = rsp->regs[rs1] & rsp->regs[rs2];
    } else {
        unsupported("R-type funct3", funct3);
    }
    if (rsp->analyze) {
        rsp->analysis.i_count += 1;
        rsp->analysis.ir_count += 1;
    }
    rsp->pc += 4; // Next instruction
}

void emu_jalr (struct rv_state_st *rsp, uint32_t iw) {
    uint32_t rs1 = (iw >> 15) & 0b1111;  // Will be ra (aka x1)
    uint64_t val = rsp->regs[rs1];  // Value of regs[1]
    if (rsp->analyze) {
        rsp->analysis.i_count += 1;
        rsp->analysis.j_count += 1;
    }
    rsp->pc = val;  // PC = return address
}

void emu_jal (struct rv_state_st *rsp, uint32_t iw) {
    uint32_t rd = rv_get_rd(iw);
    uint32_t immu21 = (get_bit(iw, 31) << 20) | (get_bits(iw, 12, 8) << 12) | (get_bit(iw, 20) << 11) | (get_bits(iw, 21, 10) << 1);
    int64_t imm = sign_extend(immu21, 20);
    if(rd != 0){
        rsp->regs[rd] = rsp->pc + 4;
    }
    if (rsp->analyze) {
        rsp->analysis.i_count += 1;
        rsp->analysis.j_count += 1;
    }
    rsp->pc += imm;
}

void emu_i_type (struct rv_state_st *rsp, uint32_t iw) {
    uint32_t rd = rv_get_rd(iw);
    uint32_t rs1 = rv_get_rs1(iw);
    uint32_t rs2 = rv_get_rs2(iw);
    uint32_t funct3 = rv_get_funct3(iw);
    uint32_t funct7 = rv_get_funct7(iw);
    uint32_t immu32 = get_bits(iw, 20, 12);
    int64_t imm = sign_extend(immu32, 11);
    if (funct3 == 0b101 && funct7 == 0b0000000) {   //SRLI
        rsp->regs[rd] = rsp->regs[rs1] >> rs2;
    } else if (funct3 == 0b000){    //ADDI
        if(rs1==0){
            rsp->regs[rd] = imm;
        } else {
            rsp->regs[rd] = rsp->regs[rs1] + imm;
        }
    } else {
        unsupported("I-type funct3", funct3);
    }
    if (rsp->analyze) {
        rsp->analysis.i_count += 1;
        rsp->analysis.ir_count += 1;
    }
    rsp->pc += 4; // Next instruction
}

void emu_64i_type (struct rv_state_st *rsp, uint32_t iw) {
    uint32_t rd = rv_get_rd(iw);
    uint32_t funct3 = rv_get_funct3(iw);
    uint32_t rs1 = rv_get_rs1(iw);
    uint32_t rs2 = rv_get_rs2(iw);
    uint32_t funct7 = rv_get_funct7(iw);
    uint32_t shamt = get_bits(rsp->regs[rs2], 0, 5);
    if (funct3 == 0b001 && funct7 == 0b0000000) { //SLLW
        int32_t res = get_bits(rsp->regs[rs1], 0, 32) << shamt;
        rsp->regs[rd] = res;
    } else if (funct3 == 0b101 && funct7 == 0b0100000) { //SRAW
        int32_t res = ((int32_t)rsp->regs[rs1] >> shamt);
        rsp->regs[rd] = res;
    } else {
        unsupported("64I-type funct3", funct3);
    }
    if (rsp->analyze) {
        rsp->analysis.i_count += 1;
        rsp->analysis.ir_count += 1;
    }
    rsp->pc += 4; // Next instruction
     
}

void emu_b_type (struct rv_state_st *rsp, uint32_t iw) {
    uint32_t rs1 = rv_get_rs1(iw);
    uint32_t rs2 = rv_get_rs2(iw);
    uint32_t funct3 = rv_get_funct3(iw);

    uint32_t immu32 = (get_bit(iw, 31) << 12) | (get_bits(iw, 25, 6) << 5) | (get_bit(iw, 7) << 11) | (get_bits(iw, 8, 4) << 1);
    int64_t imm = sign_extend(immu32, 12);  // Sign extend to 64 bits

    int64_t rs1_val = (int64_t) rsp->regs[rs1];
    int64_t rs2_val = (int64_t) rsp->regs[rs2];
    
    if ((funct3 == 0b000 && (rs1_val == rs2_val)) || //BEQ
        (funct3 == 0b100 && (rs1_val < rs2_val))  || //BLT
        (funct3 == 0b001 && (rs1_val != rs2_val)) || //BNE
        (funct3 == 0b101 && (rs1_val >= rs2_val))) { //BGE
            rsp->pc += imm;
            if (rsp->analyze) {
                rsp->analysis.i_count += 1;
                rsp->analysis.b_taken += 1;
            }
    } else if (funct3 == 0b000 || funct3 == 0b100 || funct3 == 0b001 || funct3 == 0b101) {
        rsp->pc += 4; // Next instruction
        if (rsp->analyze) {
            rsp->analysis.i_count += 1;
            rsp->analysis.b_not_taken += 1;
        }
    } else {
        unsupported("B-type funct3", funct3);
    }
}

void emu_s_type (struct rv_state_st *rsp, uint32_t iw){
    uint32_t rs1 = rv_get_rs1(iw);
    uint32_t rs2 = rv_get_rs2(iw);
    uint32_t funct3 = rv_get_funct3(iw);

    uint32_t immu32 = get_bits(iw, 25, 7) << 5 | get_bits(iw, 7, 5);
    int64_t imm = sign_extend(immu32, 11);
    uint64_t target_addr = rsp->regs[rs1]+imm;
    
    if (funct3 == 0b011) { //SD
        *((uint64_t *) target_addr) = rsp->regs[rs2];
    } else if (funct3 == 0b000) { //SB
        *((uint8_t *) target_addr) = rsp->regs[rs2];
    } else if (funct3 == 0b010) { //SW
        *((uint32_t *) target_addr) = rsp->regs[rs2];
    } else {
        unsupported("S-type funct3", funct3);
    }
    if (rsp->analyze) {
        rsp->analysis.i_count += 1;
        rsp->analysis.st_count += 1;
    }
    rsp->pc += 4; // Next instruction

}

void emu_load (struct rv_state_st *rsp, uint32_t iw) {
    uint32_t rd = rv_get_rd(iw);
    uint32_t rs1 = rv_get_rs1(iw);
    uint32_t funct3 = rv_get_funct3(iw);
    uint32_t immu32 = get_bits(iw, 20, 12);
    int64_t imm = sign_extend(immu32, 11);
    uint64_t target_addr = rsp->regs[rs1]+imm;
    if (funct3 == 0b011) { //LD
        uint64_t value = *((uint64_t *) target_addr);
        rsp->regs[rd] = value;
    } else if (funct3 == 0b010) { //LW   
        uint32_t value = *((uint32_t *) target_addr);
        rsp->regs[rd] = value;
    } else if (funct3 == 0b000) { //LB
        uint64_t target_addr = rsp->regs[rs1]+imm;
        uint8_t value = *((uint8_t *) target_addr);
        rsp->regs[rd] = value;
    } else{
        unsupported("I-type funct3", funct3);
    }
    if (rsp->analyze) {
        rsp->analysis.i_count += 1;
        rsp->analysis.ld_count += 1;
    }
    rsp->pc += 4; // Next instruction
}

static void rv_one (struct rv_state_st *rsp) {
    // uint32_t iw  = *((uint32_t*) rsp->pc);
    uint32_t iw = cache_lookup(&rsp->i_cache, (uint64_t) rsp->pc);

    uint32_t opcode = iw & 0b1111111;
#if DEBUG
    printf("iw: %08x\n", iw);
#endif

    switch (opcode) {
        case FMT_R:
            // R-type instructions have two register operands
            emu_r_type(rsp, iw);
            break;
        case FMT_I_JALR:
            // JALR (aka RET) is a variant of I-type instructions
            emu_jalr(rsp, iw);
            break;
        case FMT_J:
            emu_jal(rsp, iw);
            break;
        case FMT_I_ARITH:
            emu_i_type(rsp, iw);
            break;
        case FMT_64I_ARITH:
            emu_64i_type(rsp, iw);
            break;
        case FMT_I_LOAD:
            emu_load(rsp, iw);
            break;
        case FMT_B:
            emu_b_type(rsp, iw);
            break;
        case FMT_S:
            emu_s_type(rsp, iw);
            break;
        default:
            unsupported("Unknown opcode: ", opcode);
    }
}

uint64_t rv_emulate(struct rv_state_st *rsp) {
    while (rsp->pc != RV_STOP) {
        rv_one(rsp);
    }
    return rsp->regs[RV_A0];
}

static void print_pct(char *fmt, int numer, int denom) {
    double pct = 0.0;

    if (denom)
        pct = (double) numer / (double) denom * 100.0;
    printf(fmt, numer, pct);
}

void rv_print(struct analysis_st *ap) {
    int b_total = ap->b_taken + ap->b_not_taken;

    printf("=== Analysis\n");
    print_pct("Instructions Executed  = %d\n", ap->i_count, ap->i_count);
    print_pct("R-type + I-type        = %d (%.2f%%)\n", ap->ir_count, ap->i_count);
    print_pct("Loads                  = %d (%.2f%%)\n", ap->ld_count, ap->i_count);
    print_pct("Stores                 = %d (%.2f%%)\n", ap->st_count, ap->i_count);    
    print_pct("Jumps/JAL/JALR         = %d (%.2f%%)\n", ap->j_count, ap->i_count);
    print_pct("Conditional branches   = %d (%.2f%%)\n", b_total, ap->i_count);
    print_pct("  Branches taken       = %d (%.2f%%)\n", ap->b_taken, b_total);
    print_pct("  Branches not taken   = %d (%.2f%%)\n", ap->b_not_taken, b_total);
}
