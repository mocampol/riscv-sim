#include "cpu.hpp"

#include <sstream>
#include <stdexcept>

namespace {

// Extiende a signo un valor de 'bits' bits almacenado en los bits bajos
// de un uint32_t, devolviendo un int32_t equivalente.
int32_t signExtend(uint32_t value, int bits) {
    uint32_t signBit = 1u << (bits - 1);
    if (value & signBit) {
        // Pone en 1 todos los bits por encima de 'bits'.
        uint32_t mask = ~0u << bits;
        return static_cast<int32_t>(value | mask);
    }
    return static_cast<int32_t>(value);
}

// ---- Extraccion de campos / inmediatos segun el tipo de instruccion ----

uint32_t fieldOpcode(uint32_t i) { return i & 0x7F; }
uint32_t fieldRd(uint32_t i)     { return (i >> 7) & 0x1F; }
uint32_t fieldFunct3(uint32_t i) { return (i >> 12) & 0x7; }
uint32_t fieldRs1(uint32_t i)    { return (i >> 15) & 0x1F; }
uint32_t fieldRs2(uint32_t i)    { return (i >> 20) & 0x1F; }
uint32_t fieldFunct7(uint32_t i) { return (i >> 25) & 0x7F; }

int32_t immI(uint32_t i) {
    uint32_t v = i >> 20; // bits [31:20] quedan en los 12 bits bajos
    return signExtend(v, 12);
}

int32_t immS(uint32_t i) {
    uint32_t hi = (i >> 25) & 0x7F; // instr[31:25] -> imm[11:5]
    uint32_t lo = (i >> 7) & 0x1F;  // instr[11:7]  -> imm[4:0]
    uint32_t v = (hi << 5) | lo;
    return signExtend(v, 12);
}

int32_t immB(uint32_t i) {
    uint32_t b12   = (i >> 31) & 0x1;
    uint32_t b11   = (i >> 7) & 0x1;
    uint32_t b10_5 = (i >> 25) & 0x3F;
    uint32_t b4_1  = (i >> 8) & 0xF;
    uint32_t v = (b12 << 12) | (b11 << 11) | (b10_5 << 5) | (b4_1 << 1);
    return signExtend(v, 13);
}

int32_t immU(uint32_t i) {
    // instr[31:12] ya quedan en su posicion final; los 12 bits bajos son 0.
    return static_cast<int32_t>(i & 0xFFFFF000u);
}

int32_t immJ(uint32_t i) {
    uint32_t b20    = (i >> 31) & 0x1;
    uint32_t b19_12 = (i >> 12) & 0xFF;
    uint32_t b11    = (i >> 20) & 0x1;
    uint32_t b10_1  = (i >> 21) & 0x3FF;
    uint32_t v = (b20 << 20) | (b19_12 << 12) | (b11 << 11) | (b10_1 << 1);
    return signExtend(v, 21);
}

std::string hex32(uint32_t v) {
    std::ostringstream oss;
    oss << "0x" << std::hex << v;
    return oss.str();
}

} // namespace

Cpu::Cpu(Memory& mem) : mem_(mem), pc_(0) {
    x_.fill(0);
}

void Cpu::reset() {
    pc_ = 0;
    x_.fill(0);
    // sp (x2) apuntando al tope de la memoria (alineado a 4 bytes),
    // util para programas que usan la pila (llamadas a funciones,
    // recursion, etc.) sin que el simulador tenga que parsear el ELF.
    if (mem_.size() >= 4) {
        uint32_t top = static_cast<uint32_t>(mem_.size());
        top -= (top % 4); // alinear
        x_[2] = top - 4;
    }
}

uint32_t Cpu::getReg(unsigned idx) const {
    if (idx >= kNumRegs) throw std::out_of_range("indice de registro invalido");
    if (idx == 0) return 0;
    return x_[idx];
}

void Cpu::setReg(unsigned idx, uint32_t value) {
    if (idx >= kNumRegs) throw std::out_of_range("indice de registro invalido");
    if (idx == 0) return; // x0 esta cableado a 0
    x_[idx] = value;
}

void Cpu::step() {
    uint32_t instr = mem_.read32(pc_);
    execute(instr);
}

void Cpu::execute(uint32_t instr) {
    uint32_t opcode = fieldOpcode(instr);
    uint32_t rd     = fieldRd(instr);
    uint32_t rs1    = fieldRs1(instr);
    uint32_t rs2    = fieldRs2(instr);
    uint32_t funct3 = fieldFunct3(instr);
    uint32_t funct7 = fieldFunct7(instr);

    int32_t a = static_cast<int32_t>(getReg(rs1));
    int32_t b = static_cast<int32_t>(getReg(rs2));

    switch (opcode) {
        case 0x03: { // LOAD: lb, lh, lw, lbu, lhu
            uint32_t addr = static_cast<uint32_t>(a + immI(instr));
            int32_t val = 0;
            switch (funct3) {
                case 0x0: val = static_cast<int8_t>(mem_.read8(addr)); break;             // lb
                case 0x1: val = static_cast<int16_t>(mem_.read16(addr)); break;           // lh
                case 0x2: val = static_cast<int32_t>(mem_.read32(addr)); break;           // lw
                case 0x4: val = static_cast<int32_t>(static_cast<uint32_t>(mem_.read8(addr))); break;  // lbu
                case 0x5: val = static_cast<int32_t>(static_cast<uint32_t>(mem_.read16(addr))); break; // lhu
                default:
                    throw std::runtime_error("funct3 invalido para LOAD: " + hex32(funct3));
            }
            setReg(rd, static_cast<uint32_t>(val));
            pc_ += 4;
            break;
        }

        case 0x23: { // STORE: sb, sh, sw
            uint32_t addr = static_cast<uint32_t>(a + immS(instr));
            switch (funct3) {
                case 0x0: mem_.write8(addr, static_cast<uint8_t>(b & 0xFF)); break;   // sb
                case 0x1: mem_.write16(addr, static_cast<uint16_t>(b & 0xFFFF)); break; // sh
                case 0x2: mem_.write32(addr, static_cast<uint32_t>(b)); break;        // sw
                default:
                    throw std::runtime_error("funct3 invalido para STORE: " + hex32(funct3));
            }
            pc_ += 4;
            break;
        }

        case 0x13: { // OP-IMM: addi, slli, slti, sltiu, xori, srli, srai, ori, andi
            int32_t imm = immI(instr);
            uint32_t shamt = static_cast<uint32_t>(imm) & 0x1F;
            int32_t result = 0;
            switch (funct3) {
                case 0x0: result = a + imm; break; // addi
                case 0x1: result = static_cast<int32_t>(static_cast<uint32_t>(a) << shamt); break; // slli
                case 0x2: result = (a < imm) ? 1 : 0; break; // slti (signed)
                case 0x3: result = (static_cast<uint32_t>(a) < static_cast<uint32_t>(imm)) ? 1 : 0; break; // sltiu
                case 0x4: result = a ^ imm; break; // xori
                case 0x5: // srli / srai (bit 30 del instr, == funct7 bit 5, distingue)
                    if (funct7 & 0x20)
                        result = a >> shamt; // srai: shift aritmetico (con signo)
                    else
                        result = static_cast<int32_t>(static_cast<uint32_t>(a) >> shamt); // srli: logico
                    break;
                case 0x6: result = a | imm; break; // ori
                case 0x7: result = a & imm; break; // andi
                default:
                    throw std::runtime_error("funct3 invalido para OP-IMM: " + hex32(funct3));
            }
            setReg(rd, static_cast<uint32_t>(result));
            pc_ += 4;
            break;
        }

        case 0x37: { // LUI
            setReg(rd, static_cast<uint32_t>(immU(instr)));
            pc_ += 4;
            break;
        }

        case 0x17: { // AUIPC
            setReg(rd, pc_ + static_cast<uint32_t>(immU(instr)));
            pc_ += 4;
            break;
        }

        case 0x33: { // OP (R-type): add, sub, sll, slt, sltu, xor, srl, sra, or, and
            uint32_t shamt = static_cast<uint32_t>(b) & 0x1F;
            int32_t result = 0;
            switch (funct3) {
                case 0x0: result = (funct7 & 0x20) ? (a - b) : (a + b); break; // sub / add
                case 0x1: result = static_cast<int32_t>(static_cast<uint32_t>(a) << shamt); break; // sll
                case 0x2: result = (a < b) ? 1 : 0; break; // slt (signed)
                case 0x3: result = (static_cast<uint32_t>(a) < static_cast<uint32_t>(b)) ? 1 : 0; break; // sltu
                case 0x4: result = a ^ b; break; // xor
                case 0x5: // srl / sra
                    if (funct7 & 0x20)
                        result = a >> shamt; // sra
                    else
                        result = static_cast<int32_t>(static_cast<uint32_t>(a) >> shamt); // srl
                    break;
                case 0x6: result = a | b; break; // or
                case 0x7: result = a & b; break; // and
                default:
                    throw std::runtime_error("funct3 invalido para OP: " + hex32(funct3));
            }
            setReg(rd, static_cast<uint32_t>(result));
            pc_ += 4;
            break;
        }

        case 0x63: { // BRANCH: beq, bne, blt, bge, bltu, bgeu
            bool taken = false;
            switch (funct3) {
                case 0x0: taken = (a == b); break; // beq
                case 0x1: taken = (a != b); break; // bne
                case 0x4: taken = (a < b); break;  // blt (signed)
                case 0x5: taken = (a >= b); break; // bge (signed)
                case 0x6: taken = (static_cast<uint32_t>(a) < static_cast<uint32_t>(b)); break;  // bltu
                case 0x7: taken = (static_cast<uint32_t>(a) >= static_cast<uint32_t>(b)); break; // bgeu
                default:
                    throw std::runtime_error("funct3 invalido para BRANCH: " + hex32(funct3));
            }
            pc_ = taken ? (pc_ + static_cast<uint32_t>(immB(instr))) : (pc_ + 4);
            break;
        }

        case 0x67: { // JALR
            uint32_t target = static_cast<uint32_t>(a + immI(instr)) & ~1u;
            uint32_t link = pc_ + 4;
            pc_ = target;
            setReg(rd, link);
            break;
        }

        case 0x6F: { // JAL
            uint32_t link = pc_ + 4;
            pc_ = pc_ + static_cast<uint32_t>(immJ(instr));
            setReg(rd, link);
            break;
        }

        default:
            throw std::runtime_error("opcode no soportado: " + hex32(opcode) +
                                      " (instruccion = " + hex32(instr) + " en pc=" + hex32(pc_) + ")");
    }
}
