# riscv-sim

Simulador del ISA de RISC-V (RV32I) implementado en C++17, con interfaz
de terminal.

## ¿Qué hace?

- Mantiene el estado arquitectural de una CPU RV32I: PC, 32 registros de
  proposito general (x0..x31, con x0 cableado a 0) y una memoria plana
  byte-addressable, little-endian.
- Carga programas ya compilados (binario) en la direccion `0x00000000`.
- Permite ejecucion paso a paso, controlada desde un prompt interactivo.
- Permite inspeccionar PC, registros y memoria en cualquier punto de la
  ejecucion.
- Implementa todas las instrucciones solicitadas: loads
  (`lb/lh/lw/lbu/lhu`), stores (`sb/sh/sw`), ALU inmediata (`addi, slli,
  slti, sltiu, xori, srli, srai, ori, andi`), ALU registro-registro (`add,
  sub, sll, slt, sltu, xor, srl, sra, or, and`), `lui`, `auipc`, saltos
  condicionales (`beq, bne, blt, bge, bltu, bgeu`) y saltos incondicionales
  (`jal`, `jalr`).

No implementado: `ecall`/syscalls de SPIM, carga
directa de `.elf`, ni un desensamblador.

## Estructura del proyecto

```
riscv-sim/
├── src/
│   ├── memory.hpp / memory.cpp   // memoria plana little-endian
│   ├── cpu.hpp / cpu.cpp         // estado arquitectural + decode/execute
│   └── main.cpp                  // CLI / REPL
├── tests                         // Tests en binario
├── CMakeLists.txt
├── Makefile
└── README.md
```

## ¿Cómo compilar?

Con CMake:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

O directamente con `make` (sin CMake):

```bash
make
```

Ambas opciones generan el ejecutable `riscv-sim` (en `build/` con CMake, o en
la raiz del proyecto con `make`).

## ¿Cómo ejecutar?

```bash
./riscv-sim programa.bin
```

- `programa.bin`: archivo binario crudo a cargar en la direccion `0x0`.

Al iniciar, el simulador entra en un prompt interactivo (`>`).

## Comandos del simulador

| Comando | Descripcion |
|---|---|
| `pc` | Muestra el program counter actual. |
| `step [n]` | Ejecuta `n` instrucciones (1 por defecto). |
| `run` | Ejecuta hasta que ocurra un error o se alcance un limite de seguridad (evita colgar el simulador en un loop infinito). |
| `regs` | Muestra los 32 registros. |
| `regs x5 x14` | Muestra solo los registros indicados (acepta `x5` o `5`). |
| `mem 0x1000 0x1003` | Muestra los bytes de memoria entre las dos direcciones (inclusive). |
| `reset` | Reinicia PC y registros, y recarga el binario original desde disco. |
| `help` | Lista los comandos disponibles. |
| `exit` / `quit` | Termina el simulador. |

### Ejemplo de sesion

```
$ ./riscv-sim programa.bin
"programa.bin" cargado a memoria.
> pc
pc = 0x00000000
> step
Ejecutando instruccion.
> step
Ejecutando instruccion.
> regs x5 x14
x5 = 0x00000123
x14 = 0x10200405
> mem 0x1000 0x1003
Memoria (0x1000-0x1003): 00 05 7C E8
> exit
See you next time...
Program exited with code 0.
```

## Probar el simulador con un binario de ejemplo

El repositorio incluye un binario de prueba
(`tests/test.bin`, generado por `tests/gen_test.py`) que ejercita ALU
registro-registro, ALU inmediata, `sw`/`lw`, `beq` (tomado) y `jal`/`jalr`.
Sirve como test rápido antes de probar con los programas mas grandes
(quicksort, arbol simetrico):

```bash
./riscv-sim tests/test.bin
```

Para los programas de prueba de la tarea (quicksort, arbol simetrico), hay que compilarlos y exportarlos como binario "Raw" desde
CPUlator y cargarlos con:

```bash
./riscv-sim quicksort.bin
```

## Notas de diseno

- **Registro x2 (sp)**: al hacer `reset` (o al iniciar el simulador), `x2`
  se inicializa apuntando al tope de la memoria simulada, para que
  programas que usen la pila (llamadas a funciones, recursion, como
  quicksort) tengan un stack pointer valido sin necesitar parsear un ELF.
- **Manejo de errores**: un acceso a memoria fuera de rango o una
  instruccion con opcode no soportado lanzan una excepcion que el REPL
  captura y reporta como `Error de ejecucion: ...`, sin crashear el
  simulador ni corromper el estado (el PC y los registros solo se
  actualizan despues de que la instruccion se ejecuto sin errores).
- **`run` con limite**: como RISC-V no tiene una instruccion de "halt"
  explicita, `run` se detiene ante el primer error (por ejemplo un acceso invalido
  o opcode invalido) o tras un limite de instrucciones, para evitar que un
  loop infinito cuelgue el simulador.
