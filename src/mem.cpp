/*
 * mem.cpp
 *
 * Travis Banken
 * 2020
 *
 * Memory object and operations.
 */

/*
Memory is Big-Endian

Memory Layout
------------------------------------
0xFFF
.
.        Program/Data mem
.
0x200
------------------------------------
0x1FF
.
.           Sprites
.
0x080
------------------------------------
0x07F
.
.           Reserved Font
.
0x000
------------------------------------

*/

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <mem.h>

// Helpers
static inline bool addr_in_range(uint16_t addr)
{
    return addr <= 0xFFF;
}

// class methods
Mem::Mem()
{
    Mem::write_font();
}

void Mem::write(uint8_t data, uint16_t addr)
{
    if (addr_in_range(addr)) {
        mem[addr] = data;
    } else {
        std::cerr << "Error: Attempt to write outside of addr range!\n";
        std::exit(1);
    }
}

uint8_t Mem::read(uint16_t addr)
{
    if (addr <= 0xFFF)
        return mem[addr];
    std::cerr << "Error: Attempt to access outside of addr range!\n";
    std::exit(1);
    return 0x00;
}

void Mem::dump()
{
    std::fstream ofile;
    ofile.open("chip8-coredump", std::ios::out | std::ios::binary);
    if (!ofile.is_open()) {
        std::cerr << "Failed to create coredump file!\n";
        return;
    }
    // dump core
    for (int i = 0; i < 0xFFF; i++) {
        ofile << mem[i];
    }

    ofile.close();
}

uint32_t Mem::size()
{
    return (1024*4);
}

void Mem::write_font()
{
    // 0x0                          // 0x1
    mem[0x00] = 0xf0;               mem[0x05] = 0x20;
    mem[0x01] = 0x90;               mem[0x06] = 0x60;
    mem[0x02] = 0x90;               mem[0x07] = 0x20;
    mem[0x03] = 0x90;               mem[0x08] = 0x20;
    mem[0x04] = 0xf0;               mem[0x09] = 0x70;

    // 0x2                          // 0x3
    mem[0x0a] = 0xf0;               mem[0x0f] = 0xf0;
    mem[0x0b] = 0x10;               mem[0x10] = 0x10;
    mem[0x0c] = 0xf0;               mem[0x11] = 0xf0;
    mem[0x0d] = 0x80;               mem[0x12] = 0x10;
    mem[0x0e] = 0xf0;               mem[0x13] = 0xf0;
    
    // 0x4                          // 0x5
    mem[0x14] = 0x90;               mem[0x19] = 0xf0;
    mem[0x15] = 0x90;               mem[0x1a] = 0x80;
    mem[0x16] = 0xf0;               mem[0x1b] = 0xf0;
    mem[0x17] = 0x10;               mem[0x1c] = 0x10;
    mem[0x18] = 0x10;               mem[0x1d] = 0xf0;

    // 0x6                          // 0x7
    mem[0x1e] = 0xf0;               mem[0x23] = 0xf0;
    mem[0x1f] = 0x80;               mem[0x24] = 0x10;
    mem[0x20] = 0xf0;               mem[0x25] = 0x20;
    mem[0x21] = 0x90;               mem[0x26] = 0x40;
    mem[0x22] = 0xf0;               mem[0x27] = 0x40;
    
    // 0x8                          // 0x9
    mem[0x28] = 0xf0;               mem[0x2d] = 0xf0;
    mem[0x29] = 0x90;               mem[0x2e] = 0x90;
    mem[0x2a] = 0xf0;               mem[0x2f] = 0xf0;
    mem[0x2b] = 0x90;               mem[0x30] = 0x10;
    mem[0x2c] = 0xf0;               mem[0x31] = 0xf0;
    
    // 0xA                          // 0xB
    mem[0x32] = 0xf0;               mem[0x37] = 0xE0;
    mem[0x33] = 0x90;               mem[0x38] = 0x90;
    mem[0x34] = 0xf0;               mem[0x39] = 0xE0;
    mem[0x35] = 0x90;               mem[0x3a] = 0x90;
    mem[0x36] = 0x90;               mem[0x3b] = 0xE0;
    
    // 0xC                          // 0xD
    mem[0x3c] = 0xf0;               mem[0x41] = 0xE0;
    mem[0x3d] = 0x80;               mem[0x42] = 0x90;
    mem[0x3e] = 0x80;               mem[0x43] = 0x90;
    mem[0x3f] = 0x80;               mem[0x44] = 0x90;
    mem[0x40] = 0xf0;               mem[0x45] = 0xE0;
    
    // 0xE                          // 0xF
    mem[0x46] = 0xf0;               mem[0x4b] = 0xf0;
    mem[0x47] = 0x80;               mem[0x4c] = 0x80;
    mem[0x48] = 0xf0;               mem[0x4d] = 0xf0;
    mem[0x49] = 0x80;               mem[0x4e] = 0x80;
    mem[0x4a] = 0xf0;               mem[0x4f] = 0x80;
}
