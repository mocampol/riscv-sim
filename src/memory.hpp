#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Memoria plana, byte-addressable, little-endian, de tamaño fijo.
// No se modelan permisos ni segmentación (no se pide en la tarea).
class Memory {
public:
    explicit Memory(size_t sizeBytes);

    size_t size() const { return data_.size(); }

    // Acceso a bytes individuales (con chequeo de límites).
    uint8_t  read8(uint32_t addr) const;
    void     write8(uint32_t addr, uint8_t value);

    // Lectura/escritura little-endian de 16 y 32 bits, compuestas
    // a partir de read8/write8.
    uint16_t read16(uint32_t addr) const;
    void     write16(uint32_t addr, uint16_t value);

    uint32_t read32(uint32_t addr) const;
    void     write32(uint32_t addr, uint32_t value);

    // Pone toda la memoria en cero (usado por reset()).
    void clear();

    // Carga un archivo binario crudo en memoria a partir de baseAddr.
    // Lanza std::runtime_error si el archivo no existe o no entra en memoria.
    void loadBinary(const std::string& path, uint32_t baseAddr = 0);

private:
    std::vector<uint8_t> data_;

    void checkAddr(uint32_t addr) const;
};
