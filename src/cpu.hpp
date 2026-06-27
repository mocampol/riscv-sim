#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "memory.hpp"

// Estado arquitectural de una CPU RISC-V RV32I (PC + 32 registros)
// y la logica de fetch-decode-execute de una sola instruccion (step()).
class Cpu {
public:
    explicit Cpu(Memory& mem);

    // Reinicia PC=0 y todos los registros a 0 (x2/sp se deja apuntando
    // al tope de la memoria para que programas con funciones/recursion
    // tengan un stack valido).
    void reset();

    // Ejecuta una sola instruccion: fetch en PC, decode, execute,
    // y actualiza PC (secuencial o salto/branch).
    // Lanza std::exception derivado si la instruccion es invalida o si
    // hay un acceso a memoria fuera de rango.
    void step();

    uint32_t getPC() const { return pc_; }
    void     setPC(uint32_t value) { pc_ = value; }

    // x0 siempre devuelve 0 (invariante mantenida en setReg).
    uint32_t getReg(unsigned idx) const;
    void     setReg(unsigned idx, uint32_t value);

    static constexpr unsigned kNumRegs = 32;

private:
    Memory& mem_;
    uint32_t pc_;
    std::array<uint32_t, kNumRegs> x_;

    void execute(uint32_t instr);
};
