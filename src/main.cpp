// Simulador de RISC-V (RV32I) - Interfaz de terminal.
//
// Uso:
//   ./riscv-sim programa.bin [tamano_memoria_bytes]
//
// Comandos disponibles en el prompt interactivo:
//   pc                      - muestra el program counter
//   step [n]                - ejecuta n instrucciones (por defecto 1)
//   run                     - ejecuta hasta un error o el limite de instrucciones
//   regs [r1 r2 ...]        - muestra todos los registros, o solo los indicados
//                             (ej: regs x5 x14, tambien acepta "5" o "14")
//   mem <addr1> <addr2>     - muestra el contenido de memoria en bytes,
//                             desde addr1 hasta addr2 (inclusive)
//   reset                   - reinicia PC y registros, y recarga el binario
//   help                    - muestra esta ayuda
//   exit / quit             - termina el simulador

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "cpu.hpp"
#include "memory.hpp"

namespace {

constexpr size_t kDefaultMemSize = 1u << 20; // 1 MiB
constexpr uint64_t kRunInstrLimit = 100000000ULL; // limite de seguridad para "run"

std::string toUpperHex8(uint32_t v) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << v;
    return oss.str();
}

// Parsea una direccion/numero en decimal o hexadecimal (con prefijo 0x).
bool parseNumber(const std::string& tok, uint32_t& out) {
    try {
        size_t idx = 0;
        unsigned long val = std::stoul(tok, &idx, 0);
        if (idx != tok.size()) return false;
        out = static_cast<uint32_t>(val);
        return true;
    } catch (...) {
        return false;
    }
}

// Parsea un nombre de registro: "x5", "X5" o simplemente "5".
bool parseRegName(const std::string& tok, unsigned& out) {
    std::string t = tok;
    if (!t.empty() && (t[0] == 'x' || t[0] == 'X')) {
        t = t.substr(1);
    }
    if (t.empty()) return false;
    try {
        size_t idx = 0;
        unsigned long val = std::stoul(t, &idx, 10);
        if (idx != t.size() || val >= Cpu::kNumRegs) return false;
        out = static_cast<unsigned>(val);
        return true;
    } catch (...) {
        return false;
    }
}

void printHelp() {
    std::cout <<
        "Comandos disponibles:\n"
        "  pc                   - muestra el program counter\n"
        "  step [n]             - ejecuta n instrucciones (por defecto 1)\n"
        "  run                  - ejecuta hasta un error o limite de instrucciones\n"
        "  regs [r1 r2 ...]     - muestra registros (todos, o los indicados: x5 x14)\n"
        "  mem <addr1> <addr2>  - muestra bytes de memoria entre addr1 y addr2\n"
        "  reset                - reinicia PC, registros y recarga el binario\n"
        "  help                 - muestra esta ayuda\n"
        "  exit / quit          - termina el simulador\n";
}

void printAllRegs(const Cpu& cpu) {
    for (unsigned i = 0; i < Cpu::kNumRegs; ++i) {
        std::cout << "x" << i << (i < 10 ? "  " : " ") << "= " << toUpperHex8(cpu.getReg(i));
        if (i % 4 == 3) std::cout << "\n";
        else std::cout << "   ";
    }
    if (Cpu::kNumRegs % 4 != 0) std::cout << "\n";
}

void printRegs(const Cpu& cpu, const std::vector<std::string>& names) {
    for (const auto& name : names) {
        unsigned idx;
        if (!parseRegName(name, idx)) {
            std::cout << "Registro invalido: " << name << "\n";
            continue;
        }
        std::cout << "x" << idx << " = " << toUpperHex8(cpu.getReg(idx)) << "\n";
    }
}

void printMem(const Memory& mem, uint32_t addr1, uint32_t addr2) {
    if (addr2 < addr1) std::swap(addr1, addr2);
    std::cout << "Memoria (0x" << std::hex << addr1 << "-0x" << addr2 << "): " << std::dec;
    for (uint32_t a = addr1; a <= addr2; ++a) {
        try {
            uint8_t byte = mem.read8(a);
            std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
                       << static_cast<unsigned>(byte) << std::dec;
        } catch (const std::exception& e) {
            std::cout << "??";
        }
        if (a != addr2) std::cout << " ";
        if (a == 0xFFFFFFFFu) break; // evita overflow del bucle
    }
    std::cout << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <programa.bin> [tamano_memoria_bytes]\n";
        return 1;
    }

    std::string binPath = argv[1];
    size_t memSize = kDefaultMemSize;
    if (argc >= 3) {
        try {
            memSize = static_cast<size_t>(std::stoul(argv[2], nullptr, 0));
        } catch (...) {
            std::cerr << "Tamano de memoria invalido: " << argv[2] << "\n";
            return 1;
        }
    }

    Memory mem(memSize);
    try {
        mem.loadBinary(binPath, 0);
    } catch (const std::exception& e) {
        std::cerr << "Error cargando el binario: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\"" << binPath << "\" cargado a memoria.\n";

    Cpu cpu(mem);
    cpu.reset();

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\nSee you next time...\n";
            break;
        }

        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
        if (tokens.empty()) continue;

        const std::string& cmd = tokens[0];

        if (cmd == "pc") {
            std::cout << "pc = " << toUpperHex8(cpu.getPC()) << "\n";

        } else if (cmd == "step") {
            uint64_t n = 1;
            if (tokens.size() >= 2) {
                uint32_t val;
                if (parseNumber(tokens[1], val)) n = val;
            }
            for (uint64_t i = 0; i < n; ++i) {
                try {
                    cpu.step();
                    std::cout << "Ejecutando instruccion.\n";
                } catch (const std::exception& e) {
                    std::cout << "Error de ejecucion: " << e.what() << "\n";
                    break;
                }
            }

        } else if (cmd == "run") {
            uint64_t count = 0;
            bool stopped = false;
            while (count < kRunInstrLimit) {
                try {
                    cpu.step();
                    ++count;
                } catch (const std::exception& e) {
                    std::cout << "Error de ejecucion: " << e.what() << "\n";
                    stopped = true;
                    break;
                }
            }
            if (!stopped) {
                std::cout << "Limite de " << kRunInstrLimit
                          << " instrucciones alcanzado (posible loop infinito).\n";
            }
            std::cout << "Instrucciones ejecutadas: " << count << "\n";

        } else if (cmd == "regs") {
            if (tokens.size() == 1) {
                printAllRegs(cpu);
            } else {
                std::vector<std::string> names(tokens.begin() + 1, tokens.end());
                printRegs(cpu, names);
            }

        } else if (cmd == "mem") {
            if (tokens.size() != 3) {
                std::cout << "Uso: mem <addr1> <addr2>\n";
            } else {
                uint32_t a1, a2;
                if (!parseNumber(tokens[1], a1) || !parseNumber(tokens[2], a2)) {
                    std::cout << "Direcciones invalidas.\n";
                } else {
                    printMem(mem, a1, a2);
                }
            }

        } else if (cmd == "reset") {
            mem.clear();
            try {
                mem.loadBinary(binPath, 0);
            } catch (const std::exception& e) {
                std::cout << "Error recargando el binario: " << e.what() << "\n";
            }
            cpu.reset();
            std::cout << "Simulador reiniciado.\n";

        } else if (cmd == "help") {
            printHelp();

        } else if (cmd == "exit" || cmd == "quit") {
            std::cout << "See you next time...\n";
            std::cout << "Program exited with code 0.\n";
            break;

        } else {
            std::cout << "Comando desconocido: " << cmd << " (usa 'help')\n";
        }
    }

    return 0;
}
