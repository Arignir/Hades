/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2020 - The Hades Authors
**
\******************************************************************************/

#include "core.h"
#include "hades.h"

/*
** Implement the ADD instruction.
*/
void
core_thumb_add(
    struct core *core,
    uint16_t op
) {
    uint32_t rd;
    uint32_t rs;
    uint32_t rhs;
    bool immediate;

    rd = bitfield_get_range(op, 0, 3);
    rs = bitfield_get_range(op, 3, 6);
    immediate = bitfield_get(op, 10);

    if (immediate) {
        rhs = bitfield_get_range(op, 6, 9);
    } else {
        rhs = core->registers[bitfield_get_range(op, 6, 9)];
    }

    core->registers[rd] = core->registers[rs] + rhs;

    core->cpsr.zero = !(core->registers[rd]);
    core->cpsr.negative = bitfield_get(core->registers[rd], 31);
    core->cpsr.carry = uadd32(core->registers[rs], rhs);
    core->cpsr.overflow = iadd32(core->registers[rs], rhs);
}

/*
** Implement the SUB instruction.
*/
void
core_thumb_sub(
    struct core *core,
    uint16_t op
) {
    uint32_t rd;
    uint32_t rs;
    uint32_t rhs;
    bool immediate;

    rd = bitfield_get_range(op, 0, 3);
    rs = bitfield_get_range(op, 3, 6);
    immediate = bitfield_get(op, 10);

    if (immediate) {
        rhs = bitfield_get_range(op, 6, 9);
    } else {
        rhs = core->registers[bitfield_get_range(op, 6, 9)];
    }

    core->registers[rd] = core->registers[rs] - rhs;

    core->cpsr.zero = !(core->registers[rd]);
    core->cpsr.negative = bitfield_get(core->registers[rd], 31);
    core->cpsr.carry = usub32(core->registers[rs], rhs);
    core->cpsr.overflow = isub32(core->registers[rs], rhs);
}

/*
** Implement the ADD immediate instruction.
*/
void
core_thumb_add_imm(
    struct core *core,
    uint16_t op
) {
    uint16_t rd;
    uint32_t imm;

    rd = bitfield_get_range(op, 8, 11);
    imm = bitfield_get_range(op, 0, 8);

    core->cpsr.carry = uadd32(core->registers[rd], imm);
    core->cpsr.overflow = iadd32(core->registers[rd], imm);

    core->registers[rd] += imm;

    core->cpsr.zero = !(core->registers[rd]);
    core->cpsr.negative = bitfield_get(core->registers[rd], 31);
}

/*
** Implement the SUB immediate instruction.
*/
void
core_thumb_sub_imm(
    struct core *core,
    uint16_t op
) {
    uint16_t rd;
    uint32_t imm;

    rd = bitfield_get_range(op, 8, 11);
    imm = bitfield_get_range(op, 0, 8);

    core->cpsr.carry = usub32(core->registers[rd], imm);
    core->cpsr.overflow = isub32(core->registers[rd], imm);

    core->registers[rd] -= imm;

    core->cpsr.zero = !(core->registers[rd]);
    core->cpsr.negative = bitfield_get(core->registers[rd], 31);
}

/*
** Implement the ADD High Register instruction.
*/
void
core_thumb_add_reg(
    struct core *core,
    uint16_t op
) {
    uint16_t rd;
    uint16_t rs;
    bool h1;
    bool h2;

    h1 = bitfield_get(op, 7);
    h2 = bitfield_get(op, 6);
    rd = bitfield_get_range(op, 0, 3) + h1 * 8;
    rs = bitfield_get_range(op, 3, 6) + h2 * 8;

    hs_assert(h1 | h2); // Ensure h1 != 0 && h2 != 0, or op is undefined.

    core->registers[rd] = core->registers[rd] + core->registers[rs];
}

/*
** Implement the Load address from SP instruction.
*/
void
core_thumb_add_from_sp(
    struct core *core,
    uint16_t op
) {
    uint32_t offset;
    uint32_t rd;

    offset = bitfield_get_range(op, 0, 8) << 2;
    rd = bitfield_get_range(op, 8, 11);

    core->registers[rd] = core->sp + offset;
}

/*
** Implement the Load address from PC instruction.
*/
void
core_thumb_add_from_pc(
    struct core *core,
    uint16_t op
) {
    uint32_t offset;
    uint32_t rd;

    offset = bitfield_get_range(op, 0, 8) << 2;
    rd = bitfield_get_range(op, 8, 11);

    core->registers[rd] = (core->pc & 0xFFFFFFFC) + offset;
}

/*
** Implement a bunch of ALU instructions.
*/
void
core_thumb_alu(
    struct core *core,
    uint16_t op
) {
    uint16_t rd;
    uint16_t rs;
    uint32_t op1;
    uint32_t op2;
    bool carry_out;

    rd = bitfield_get_range(op, 0, 3);
    rs = bitfield_get_range(op, 3, 6);

    op1 = core->registers[rd];
    op2 = core->registers[rs];

    switch (bitfield_get_range(op, 6, 13)) {
        case 0b0000:
            // AND
            core->registers[rd] = op1 & op2;
            core->cpsr.zero = !(core->registers[rd]);
            core->cpsr.negative = bitfield_get(core->registers[rd], 31);
            break;
        case 0b0001:
            // EOR (XOR)
            core->registers[rd] = op1 ^ op2;
            core->cpsr.zero = !(core->registers[rd]);
            core->cpsr.negative = bitfield_get(core->registers[rd], 31);
            break;
        case 0b0010:
            // LSL (Logical Shift Left)

            op2 &= 0xFF; // Keep only one byte

            switch (op2) {
                case 0:
                    carry_out = core->cpsr.carry;
                    break;
                case 1 ... 32:
                    op1 <<= op2 - 1;
                    carry_out = op1 >> 31;
                    op1 <<= 1;
                    break;
                default:
                    op1 = 0;
                    carry_out = 0;
                    break;
            }

            core->cpsr.carry = carry_out;
            core->cpsr.zero = !op1;
            core->cpsr.negative = bitfield_get(op1, 31);

            core->registers[rd] = op1;
            break;
        case 0b0011:
            // LSR (Logical Shift Right)

            op2 &= 0xFF; // Keep only one byte

            switch (op2) {
                case 0:
                    carry_out = core->cpsr.carry;
                    break;
                case 1 ... 32:
                    op1 >>= op2 - 1;
                    carry_out = op1 & 0b1;
                    op1 >>= 1;
                    break;
                default:
                    op1 = 0;
                    carry_out = 0;
                    break;
            }

            core->cpsr.carry = carry_out;
            core->cpsr.zero = !op1;
            core->cpsr.negative = bitfield_get(op1, 31);

            core->registers[rd] = op1;
            break;
        case 0b0100:
            // ASR (Arithmetic Shift Right)

            op2 &= 0xFF; // Keep only one byte

            switch (op2) {
                case 0:
                    carry_out = core->cpsr.carry;
                    break;
                case 1 ... 32:
                    op1 = (int32_t)op1 >> (op2 - 1);
                    carry_out = op1 & 0b1;
                    op1 = (int32_t)op1 >> 1;
                    break;
                default:
                    carry_out = bitfield_get(op1, 31);
                    op1 = carry_out ? 0xFFFFFFFF : 0;
                    break;
            }

            core->cpsr.carry = carry_out;
            core->cpsr.zero = !op1;
            core->cpsr.negative = bitfield_get(op1, 31);

            core->registers[rd] = op1;
            break;
        case 0b0101:
            // ADC (Add with Carry) (op1 + op2 + carry)
            core->registers[rd] = op1 + op2 + core->cpsr.carry;
            core->cpsr.zero = (core->registers[rd] == 0);
            core->cpsr.negative = ((core->registers[rd] & 0x80000000) != 0);
            core->cpsr.carry = uadd32(op1, op2);
            core->cpsr.carry |= uadd32(op1 + op2, core->cpsr.carry);
            core->cpsr.overflow = iadd32(op1, op2);
            core->cpsr.overflow = iadd32(op1 + op2, core->cpsr.carry);
            break;
        case 0b0110:
            // SBC (Sub with carry) (op1 - op2 + carry - 1)
            core->registers[rd] = op1 - op2 + core->cpsr.carry - 1;
            core->cpsr.zero = !(core->registers[rd]);
            core->cpsr.negative = bitfield_get(core->registers[rd], 31);
            core->cpsr.carry = usub32(op1, op2);
            core->cpsr.carry |= uadd32(op1 - op2, core->cpsr.carry);
            core->cpsr.carry |= usub32(op1 - op2 + core->cpsr.carry, 1);
            core->cpsr.overflow = isub32(op1, op2);
            core->cpsr.overflow = iadd32(op1 - op2, core->cpsr.carry);
            core->cpsr.overflow |= isub32(op1 - op2 + core->cpsr.carry, 1);
            break;
        case 0b0111:
            // ROR (Rotate Right)
            op2 &= 0xFF; // Keep only one byte

            if (op2 > 32) {
                op2 %= 32;
            }

            switch (op2) {
                case 0:
                    carry_out = core->cpsr.carry;
                    break;
                case 1 ... 31:
                    carry_out = (op1 >> (op2 - 1)) & 0b1;
                    op1 = (op1 >> op2) | (op1 << (32 - op2));
                    break;
                case 32:
                    carry_out = (op1 >> 31) & 0b1;
                    break;
            }

            core->cpsr.carry = carry_out;
            core->cpsr.zero = !op1;
            core->cpsr.negative = bitfield_get(op1, 31);

            core->registers[rd] = op1;
            break;
        case 0b1000:
            // TST (as AND, but result is not written)
            core->cpsr.zero = !(op1 & op2);
            core->cpsr.negative = bitfield_get(op1 & op2, 31);
            break;
        case 0b1001:
            // NEG (As 0 - op2, implemented as RSBS Rd, Rs, #0)
            core->registers[rd] = 0 - op2;
            core->cpsr.zero = !(core->registers[rd]);
            core->cpsr.negative = bitfield_get(core->registers[rd], 31);
            core->cpsr.carry = usub32(0, op2);
            core->cpsr.overflow = isub32(0, op2);
            break;
        case 0b1010:
            // CMP (as SUB, but result is not written)
            core->cpsr.zero = !(op1 - op2);
            core->cpsr.negative = bitfield_get(op1 - op2, 31);
            core->cpsr.carry = usub32(op1, op2);
            core->cpsr.overflow = isub32(op1, op2);
            break;
        case 0b1011:
            // CMN (as ADD, but result is not written)
            core->cpsr.zero = !(op1 + op2);
            core->cpsr.negative = bitfield_get(op1 + op2, 31);
            core->cpsr.carry = uadd32(op1, op2);
            core->cpsr.overflow = iadd32(op1, op2);
            break;
        case 0b1100:
            // ORR (Logical OR)
            core->registers[rd] = op1 | op2;
            core->cpsr.zero = !(core->registers[rd]);
            core->cpsr.negative = bitfield_get(core->registers[rd], 31);
            break;
        case 0b1101:
            panic(CORE, "Thumb ALU operation MUL not implemented yet.");
            break;
        case 0b1110:
            // BIC (op1 AND NOT op2)
            core->registers[rd] = op1 & ~op2;
            core->cpsr.zero = !(core->registers[rd]);
            core->cpsr.negative = bitfield_get(core->registers[rd], 31);
            break;
        case 0b1111:
            // MVN (NOT op2, op1 is ignored)
            core->registers[rd] = ~op2;
            core->cpsr.zero = !(core->registers[rd]);
            core->cpsr.negative = bitfield_get(core->registers[rd], 31);
            break;
        default:
            panic(CORE, "Thumb ALU operation %x not implemented yet.", bitfield_get_range(op, 6, 13));
            break;
    }
}

/*
** Implement the CMP from register instruction.
*/
void
core_thumb_cmp_reg(
    struct core *core,
    uint16_t op
) {
    uint16_t rd;
    uint16_t rs;
    bool h1;
    bool h2;
    uint32_t op1;
    uint32_t op2;

    h1 = bitfield_get(op, 7);
    h2 = bitfield_get(op, 6);
    rd = bitfield_get_range(op, 0, 3) + h1 * 8;
    rs = bitfield_get_range(op, 3, 6) + h2 * 8;
    op1 = core->registers[rd];
    op2 = core->registers[rs];

    hs_assert(h1 | h2); // Ensure h1 != 0 && h2 != 0, or op is undefined.

    core->cpsr.zero = !(op1 - op2);
    core->cpsr.negative = bitfield_get(op1 - op2, 31);
    core->cpsr.carry = usub32(op1, op2);
    core->cpsr.overflow = isub32(op1, op2);
}

/*
** Implement the MOV from register instruction.
*/
void
core_thumb_mov_reg(
    struct core *core,
    uint16_t op
) {
    uint16_t rd;
    uint16_t rs;
    bool h1;
    bool h2;

    h1 = bitfield_get(op, 7);
    h2 = bitfield_get(op, 6);
    rd = bitfield_get_range(op, 0, 3) + h1 * 8;
    rs = bitfield_get_range(op, 3, 6) + h2 * 8;

    hs_assert(h1 | h2); // Ensure h1 != 0 && h2 != 0, or op is undefined.

    core->registers[rd] = core->registers[rs];

    if (rd == 15) {
        core->pc &= 0xfffffffe;
        core_reload_pipeline(core);
    }
}

/*
** Implement the MOV from immediate instruction.
*/
void
core_thumb_mov_imm(
    struct core *core,
    uint16_t op
) {
    uint16_t rd;
    uint32_t imm;

    rd = bitfield_get_range(op, 8, 11);
    imm = bitfield_get_range(op, 0, 8);

    core->registers[rd] = imm;
    core->cpsr.zero = !(core->registers[rd]);
    core->cpsr.negative = bitfield_get(core->registers[rd], 31); // Useless ?
}

/*
** Implement the ADD offset to stack pointer instruction.
*/
void
core_thumb_add_sp(
    struct core *core,
    uint16_t op
) {
    bool sign;
    uint32_t offset;

    sign = bitfield_get(op, 7);
    offset = bitfield_get_range(op, 0, 7) << 2;

    if (sign) {
        // Offset is negative
        core->sp -= offset;
    } else {
        // Offset is positive
        core->sp += offset;
    }
}

/*
** Implement the Compare Immediate instructions.
*/
void
core_thumb_cmp_imm(
    struct core *core,
    uint16_t op
) {
    uint16_t rd;
    uint32_t imm;
    uint32_t tmp;

    rd = bitfield_get_range(op, 8, 11);
    imm = bitfield_get_range(op, 0, 8);

    tmp = core->registers[rd] - imm;

    core->cpsr.zero = !tmp;
    core->cpsr.negative = bitfield_get(tmp, 31);
    core->cpsr.carry = usub32(core->registers[rd], imm);
    core->cpsr.overflow = isub32(core->registers[rd], imm);
}
