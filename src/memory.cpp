#include "memory.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

Memory::Memory(size_t sizeBytes) : data_(sizeBytes, 0) {}

void Memory::checkAddr(uint32_t addr) const {
    if (static_cast<size_t>(addr) >= data_.size()) {
        std::ostringstream oss;
        oss << "acceso a memoria fuera de rango: 0x" << std::hex << addr
            << " (tamano de memoria = 0x" << data_.size() << ")";
        throw std::out_of_range(oss.str());
    }
}

uint8_t Memory::read8(uint32_t addr) const {
    checkAddr(addr);
    return data_[addr];
}

void Memory::write8(uint32_t addr, uint8_t value) {
    checkAddr(addr);
    data_[addr] = value;
}

uint16_t Memory::read16(uint32_t addr) const {
    uint16_t b0 = read8(addr);
    uint16_t b1 = read8(addr + 1);
    return static_cast<uint16_t>(b0 | (b1 << 8));
}

void Memory::write16(uint32_t addr, uint16_t value) {
    write8(addr, static_cast<uint8_t>(value & 0xFF));
    write8(addr + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
}

uint32_t Memory::read32(uint32_t addr) const {
    uint32_t b0 = read8(addr);
    uint32_t b1 = read8(addr + 1);
    uint32_t b2 = read8(addr + 2);
    uint32_t b3 = read8(addr + 3);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void Memory::write32(uint32_t addr, uint32_t value) {
    write8(addr, static_cast<uint8_t>(value & 0xFF));
    write8(addr + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
    write8(addr + 2, static_cast<uint8_t>((value >> 16) & 0xFF));
    write8(addr + 3, static_cast<uint8_t>((value >> 24) & 0xFF));
}

void Memory::clear() {
    std::fill(data_.begin(), data_.end(), uint8_t{0});
}

void Memory::loadBinary(const std::string& path, uint32_t baseAddr) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("no se pudo abrir el archivo: " + path);
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize < 0) {
        throw std::runtime_error("no se pudo determinar el tamano del archivo: " + path);
    }

    if (static_cast<size_t>(baseAddr) + static_cast<size_t>(fileSize) > data_.size()) {
        throw std::runtime_error("el programa no entra en la memoria del simulador");
    }

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data_.data()) + baseAddr, fileSize);
    if (!file) {
        throw std::runtime_error("error leyendo el archivo: " + path);
    }
}
