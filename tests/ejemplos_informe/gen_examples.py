import struct

def R(funct7,rs2,rs1,funct3,rd,opcode):
    return (funct7<<25)|(rs2<<20)|(rs1<<15)|(funct3<<12)|(rd<<7)|opcode

def I(imm,rs1,funct3,rd,opcode):
    imm &= 0xFFF
    return (imm<<20)|(rs1<<15)|(funct3<<12)|(rd<<7)|opcode

def S(imm,rs2,rs1,funct3,opcode):
    imm &= 0xFFF
    imm11_5 = (imm>>5)&0x7F
    imm4_0 = imm&0x1F
    return (imm11_5<<25)|(rs2<<20)|(rs1<<15)|(funct3<<12)|(imm4_0<<7)|opcode

def B(imm,rs2,rs1,funct3,opcode):
    imm &= 0x1FFF
    b12=(imm>>12)&1; b11=(imm>>11)&1; b10_5=(imm>>5)&0x3F; b4_1=(imm>>1)&0xF
    return (b12<<31)|(b10_5<<25)|(rs2<<20)|(rs1<<15)|(funct3<<12)|(b4_1<<8)|(b11<<7)|opcode

def J(imm,rd,opcode):
    imm &= 0x1FFFFF
    b20=(imm>>20)&1; b19_12=(imm>>12)&0xFF; b11=(imm>>11)&1; b10_1=(imm>>1)&0x3FF
    return (b20<<31)|(b10_1<<21)|(b11<<20)|(b19_12<<12)|(rd<<7)|opcode

def write(name, instrs):
    with open(name,'wb') as f:
        for ins in instrs:
            f.write(struct.pack('<I', ins & 0xFFFFFFFF))
    print(name, len(instrs)*4, "bytes")

# Ejemplo 1: ALU registro-registro e inmediata
ex1 = [
    I(7,0,0b000,5,0b0010011),    # addi x5,x0,7
    I(3,0,0b000,6,0b0010011),    # addi x6,x0,3
    R(0,6,5,0b000,7,0b0110011),  # add  x7,x5,x6
    R(0b0100000,6,5,0b000,8,0b0110011), # sub x8,x5,x6
    R(0,6,5,0b111,9,0b0110011),  # and  x9,x5,x6
    R(0,6,5,0b110,10,0b0110011), # or   x10,x5,x6
]
write('ex1_alu.bin', ex1)

# Ejemplo 2: store/load de palabra
ex2 = [
    I(0x7B,0,0b000,5,0b0010011),     # addi x5,x0,123
    S(0x40,5,0,0b010,0b0100011),     # sw x5,0x40(x0)
    I(0x40,0,0b010,6,0b0000011),     # lw x6,0x40(x0)
]
write('ex2_mem.bin', ex2)

# Ejemplo 3: branch beq (tomado)
ex3 = [
    I(5,0,0b000,5,0b0010011),    # addi x5,x0,5      addr0
    I(5,0,0b000,6,0b0010011),    # addi x6,x0,5      addr4
    B(8,6,5,0b000,0b1100011),    # beq x5,x6,+8      addr8 -> target addr16
    I(999,0,0b000,7,0b0010011),  # addi x7,x0,999    addr12 (saltada)
    I(111,0,0b000,7,0b0010011),  # addi x7,x0,111    addr16 (destino)
]
write('ex3_branch.bin', ex3)

# Ejemplo 4: jal/jalr simulando llamada a funcion
ex4 = [
    I(5,0,0b000,10,0b0010011),     # addi x10,x0,5         addr0
    J(12,1,0b1101111),             # jal x1,+12            addr4  -> target addr16
    I(1,0,0b000,11,0b0010011),     # addi x11,x0,1         addr8  (retorno aqui)
    I(0,0,0b000,0,0b0010011),      # nop                   addr12
    I(99,0,0b000,12,0b0010011),    # addi x12,x0,99        addr16 (cuerpo funcion)
    I(0,1,0b000,0,0b1100111),      # jalr x0,x1,0          addr20 (retorna a addr8)
]
write('ex4_jal.bin', ex4)
