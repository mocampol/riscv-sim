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

instrs = []
# addi x5, x0, 5
instrs.append(I(5,0,0b000,5,0b0010011))
# addi x6, x0, 10
instrs.append(I(10,0,0b000,6,0b0010011))
# add x7, x5, x6   -> x7 = 15
instrs.append(R(0,6,5,0b000,7,0b0110011))
# sub x8, x6, x5   -> x8 = 5
instrs.append(R(0b0100000,5,6,0b000,8,0b0110011))
# sw x7, 256(x0)   -> mem[0x100] = 15
instrs.append(S(256,7,0,0b010,0b0100011))
# lw x9, 256(x0)   -> x9 = 15
instrs.append(I(256,0,0b010,9,0b0000011))
# beq x9, x7, +8   -> deberia saltar (igual), salta 2 instrucciones adelante
instrs.append(B(8,7,9,0b000,0b1100011))
# addi x10, x0, 999  (NO deberia ejecutarse si el branch funciona)
instrs.append(I(999,0,0b000,10,0b0010011))
# addi x11, x0, 111  (destino del branch, idx=7 -> pc = pc_branch+8)
instrs.append(I(111,0,0b000,11,0b0010011))
# jal x1, +8 (salta 2 instrucciones, guarda return addr en x1)
instrs.append(J(8,1,0b1101111))
# addi x12, x0, 222 (NO deberia ejecutarse, salteada por jal)
instrs.append(I(222,0,0b000,12,0b0010011))
# addi x13, x0, 77  (destino del jal)
instrs.append(I(77,0,0b000,13,0b0010011))
# jalr x14, x1, 0   -> salta de vuelta a la addr guardada en x1 (la instr addi x12...)
instrs.append(I(0,1,0b000,14,0b1100111))
# (si jalr funciona bien, este addi x12 SI deberia ejecutarse ahora)
# nop (addi x0,x0,0) para terminar
instrs.append(I(0,0,0b000,0,0b0010011))

with open('test.bin','wb') as f:
    for instr in instrs:
        f.write(struct.pack('<I', instr & 0xFFFFFFFF))

print(f"{len(instrs)} instrucciones escritas, {len(instrs)*4} bytes")
