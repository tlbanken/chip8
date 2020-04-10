/*
 * chip8.cpp
 */

/*
OPCODES
0NNN -- Call RCA 1802 program at addr NNN. Not necessary for most ROMs
00E0 -- clear the screen
00EE -- return from subroutine
1NNN -- jmp to adr NNN
2NNN -- call subroutine at NNN
3XNN -- skip next instr if VX == NN
4XNN -- skip next instr if VX != NN
5XY0 -- skip next instr if VX == VY
6XNN -- set VX to NN
7XNN -- Add NN to VX (carry flag not changed)

8XY0 -- set VX == VY
8XY1 -- VX = VX | VY
8XY2 -- VX = VX & VY
8XY3 -- VX = VX ^ VY
8XY4 -- VX = VX + VY (VF set to 1 if carry out, 0 if not)
8XY5 -- VX = VX - VY (VF set to 0 if borrow, 1 if not)
8XY6 -- VX = VX >> 1 (Store least sig bit of VX in VF before shift)
8XY7 -- VX = VY - VX (VF set to 0 if  borrow, 1 if not)
8XY8 -- VX = VX << 1 (Store most sig bit of VX in VF before shift)

9XY0 -- skip next isntr if VX != VY
ANNN -- set I to addr NNN
BNNN -- Jmp to addr NNN + V0
CXNN -- VX = rand() & NN
DXYN -- Draw sprite at coordinate (VX,VY) with width 8 pixels and height N pixels

EX9E -- Skip next instr if key stored in VX is pressed
EXA1 -- Skip next instr if key stored in VX isn't pressed

FX07 -- Set VX to the value of the delay timer.
FX0A -- Key press is awaited, then stored in VX
FX15 -- Sets the delay timer to VX
FX18 -- Sets the sound timer to VX
FX1E -- Adds VX to I. VF is set to 1 when there is a range overflow (I+VX > 0xFFF),
        and 0 otherwise
FX29 -- Sets I to the location of the sprite for the character in VX. Characters
        0-F are represented by a 4x5 font
FX33 -- take the decimal representation of VX, place the hundreds digit in memory
        at location in I, the tens digit at location I+1, and the ones digit at 
        location I+2
FX55 -- Store V0 to VX (inclusive) in mem starting at addr I.
FX65 -- Fill V0 to VX (inclusive) in mem starting at addr I.
*/

#include <fstream>
#include <iostream>
#include <chip8.h>
#include <assert.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>

#define INSTR_LOG 0
#define OP_LOG 1

static void log_instr(Instr i)
{
    std::fprintf(stderr, "----------------------------------------\n");
    std::fprintf(stderr, "Current Instruction: 0x%04X\n", i.raw);
    std::fprintf(stderr, "op  -> 0x%01X\n", i.op);
    std::fprintf(stderr, "nnn -> 0x%03X\n", i.nnn);
    std::fprintf(stderr, "nn  -> 0x%02X\n", i.nn);
    std::fprintf(stderr, "vx  -> 0x%01X\n", i.vx);
    std::fprintf(stderr, "vy  -> 0x%01X\n", i.vy);
    std::fprintf(stderr, "----------------------------------------\n");
}

Chip8::Chip8(std::ifstream& program)
    : I(0), pc(0x200), mem(), periphs("Chip8", 16)
{
    // set seed for rand
    std::srand(std::time(nullptr));

    // load ops into array
    opfuncs[0]  = &Chip8::op0;
    opfuncs[1]  = &Chip8::op1;
    opfuncs[2]  = &Chip8::op2;
    opfuncs[3]  = &Chip8::op3;
    opfuncs[4]  = &Chip8::op4;
    opfuncs[5]  = &Chip8::op5;
    opfuncs[6]  = &Chip8::op6;
    opfuncs[7]  = &Chip8::op7;
    opfuncs[8]  = &Chip8::op8;
    opfuncs[9]  = &Chip8::op9;
    opfuncs[10] = &Chip8::opA;
    opfuncs[11] = &Chip8::opB;
    opfuncs[12] = &Chip8::opC;
    opfuncs[13] = &Chip8::opD;
    opfuncs[14] = &Chip8::opE;
    opfuncs[15] = &Chip8::opF;

    // load program
    Chip8::load_program(program);
}

void Chip8::load_program(std::ifstream& program)
{
    // TODO
    uint16_t addr = 0x200;
    uint16_t offset = 0;
    while (!program.eof()) {
        uint8_t byte;
        program.get((char&)byte);
        mem.write(byte, (addr + offset));
        offset++;
    }
}

void Chip8::run()
{
    while (pc < mem.size()) {
        step();
    }
    while (1)
        periphs.poll();
}

void Chip8::step()
{
    std::fprintf(stderr, "========================================\n");
    std::fprintf(stderr, "Current PC: 0x%04X\n", pc);
    assert(pc < mem.size() && pc >= 0x200);

    // read instruction
    // instr are 2 bytes in size (requires 2 reads)
    uint16_t raw_instr = (((uint16_t)mem.read(pc)) << 8) | mem.read(pc+1);
    if (raw_instr == 0x0) {
        nop();
        return;
    }
    Instr instr;
    instr.raw =  raw_instr;
    instr.op  = (raw_instr >> 12) & 0xF;
    instr.nnn = (raw_instr >>  0) & 0xFFF;
    instr.nn  = (raw_instr >>  0) & 0xFF;
    instr.n   = (raw_instr >>  0) & 0xF;
    instr.vx  = (raw_instr >>  8) & 0xF;
    instr.vy  = (raw_instr >>  4) & 0xF;
    log_instr(instr);

    assert(instr.op < NUM_OPS);
    assert(opfuncs[instr.op] != NULL);
    // call op function
    (this->*opfuncs[instr.op])(instr);
    // periphs.place_pixel(0,0,1);
    periphs.refresh();
}

void Chip8::nop()
{
    std::fprintf(stderr, "NOP Instruction!\n");
    pc += 2;
}

// *** Op Code handlers ***

void Chip8::op0(Instr instr)
{
    switch (instr.raw) {
    case 0x00E0:
        // 00E0 -- clear the screen
        periphs.clear_screen();
        break;
    case 0x00EE:
        // 00EE -- return from subroutine
        std::cerr << "Warning: Instruction 00EE not implemented :(\n";
        break;
    default:
        // 0NNN -- Call RCA 1802 program at addr NNN. 
        // Not necessary for most ROMs
        std::cerr << "Warning: Instruction 0NNN not implemented :(\n";
        break;
    }
    pc += 2;
}

void Chip8::op1(Instr instr)
{
    std::fprintf(stderr, "Jumping to 0x%04x\n", instr.nnn);
    // 1NNN -- jmp to adr NNN
    pc = instr.nnn;
}

void Chip8::op2(Instr instr)
{
    (void) instr;
    // TODO
    // 2NNN -- call subroutine at NNN 
    std::cerr << "Warning: Instruction 2NNN not implemented :(\n";
    pc += 2;
}

void Chip8::op3(Instr instr)
{
    // 3XNN -- skip next instr if VX == NN
    if (V[instr.vx] == instr.nn) {
        pc += 4;
    } else {
        pc += 2;
    }
}

void Chip8::op4(Instr instr)
{
    // 4XNN -- skip next instr if VX != NN
    if (V[instr.vx] != instr.nn) {
        pc += 4;
    } else {
        pc += 2;
    }
}

void Chip8::op5(Instr instr)
{
    // 5XY0 -- skip next instr if VX == VY
    if (V[instr.vx] == V[instr.vy]) {
        pc += 4;
    } else {
        pc += 2;
    }
}

void Chip8::op6(Instr instr)
{
    // 6XNN -- set VX to NN
    V[instr.vx] = instr.nn;
    pc += 2;
}

void Chip8::op7(Instr instr)
{
    // 7XNN -- Add NN to VX (carry flag not changed)
    V[instr.vx] += instr.nn;
    // V[instr.vx] &= 0xF; // TODO maybe this???
    pc += 2;
}

void Chip8::op8(Instr instr)
{
    uint16_t res;
    switch (instr.raw & 0xF) {
    case 0:
        // 8XY0 -- set VX = VY
        V[instr.vx] = V[instr.vy];
        break;
    case 1:
        // 8XY1 -- VX = VX | VY
        V[instr.vx] |= V[instr.vy];
        break;
    case 2:
        // 8XY2 -- VX = VX & VY
        V[instr.vx] &= V[instr.vy];
        break;
    case 3:
        // 8XY3 -- VX = VX ^ VY
        V[instr.vx] ^= V[instr.vy];
        break;
    case 4:
        // 8XY4 -- VX = VX + VY (VF set to 1 if carry out, 0 if not)
        res = V[instr.vx] + V[instr.vy];
        V[instr.vx] = res & 0xFF;
        V[0xF] = (res >> 16) & 0x1;
        break;
    case 5:
        // 8XY5 -- VX = VX - VY (VF set to 0 if borrow, 1 if not)
        V[0xF] = V[instr.vx] < V[instr.vy] ? 0 : 1;
        V[instr.vx] -= V[instr.vy];
        break;
    case 6:
        // 8XY6 -- VX = VX >> 1 (Store least sig bit of VX in VF before shift)
        V[0xF] = V[instr.vx] & 0x1;
        V[instr.vx] = V[instr.vx] >> 1;
        break;
    case 7:
        // 8XY7 -- VX = VY - VX (VF set to 0 if borrow, 1 if not)
        V[0xF] = V[instr.vy] < V[instr.vx] ? 0 : 1;
        V[instr.vy] -= V[instr.vx];
        break;
    case 8:
        // 8XY8 -- VX = VX << 1 (Store most sig bit of VX in VF before shift)
        // NOTE what happens if instr.vx == 0xF?
        V[0xF] = (V[instr.vx] >> 7) & 0x1;
        V[instr.vx] = V[instr.vx] << 1;
        break;
    default:
        std::cerr << __FUNCTION__ << ": Error: This line shouldn't print!\n";
        std::exit(1);
    }
    pc += 2;
}

void Chip8::op9(Instr instr)
{
    // 9XY0 -- skip next isntr if VX != VY
    if (V[instr.vx] != V[instr.vy]) {
        pc += 4;
    } else {
       pc += 2;
    }
}

void Chip8::opA(Instr instr)
{
    // ANNN -- set I to addr NNN
    I = instr.nnn;
    pc += 2;
}

void Chip8::opB(Instr instr)
{
    // BNNN -- Jmp to addr NNN + V0
    pc += instr.nnn + V[0];
}

void Chip8::opC(Instr instr)
{
    // CXNN -- VX = rand() & NN
    V[instr.vx] = std::rand() & instr.nn;
    pc += 2;
}

void Chip8::opD(Instr instr)
{
    // DXYN -- Draw sprite at coordinate 
    // (VX,VY) with width 8 pixels and height N pixels, with
    // sprite loaded at adrr I
    // set VF to 1 if any pixels unset, 00 otherwise
    uint y = V[instr.vy];
    bool collision = false;
    for (int i = 0; i < instr.n; i++) {
        uint8_t row = mem.read(I+i);
        int x = V[instr.vx];
        for (int pix = 0; pix < 8; pix++) {
            uint8_t pixval = (row >> (7-pix)) & 0x1;
            // std::cerr << "Pixval: " << (int) pixval << std::endl;
            collision = collision || periphs.place_pixel(x, y, pixval);
            x++;
        }
        y++;
    }
    V[0xF] = collision ? 1 : 0;

    pc += 2;
}

void Chip8::opE(Instr instr)
{
    switch (instr.raw & 0xFF) {
    case 0x9E:
        // EX9E -- Skip next instr if key stored in VX is pressed
        // TODO
        std::cerr << "Warning: Keyboard not setup!\n";
        break;
    case 0xA1:
        // EXA1 -- Skip next instr if key stored in VX isn't pressed
        // TODO
        std::cerr << "Warning: Keyboard not setup!\n";
        break;
    default:
        std::cerr << "Error (opE): Unknown instruction!\n";
        std::exit(1);
    }
    pc += 2;
}

void Chip8::opF(Instr instr)
{
    uint16_t res;
    switch (instr.raw & 0xFF) {
    case 0x07:
        // FX07 -- Set VX to the value of the delay timer.
        std::cerr << "Warning: No delay timer!\n";
        break;
    case 0x0A:
        // FX0A -- Key press is awaited, then stored in VX
        std::cerr << "Warning: No key await implemented!\n";
        break;
    case 0x15:
        // FX15 -- Sets the delay timer to VX
        std::cerr << "Warning: No delay timer!\n";
        break;
    case 0x18:
        // FX18 -- Sets the sound timer to VX
        std::cerr << "Warning: No sound timer!\n";
        break;
    case 0x1E:
        // FX1E -- Adds VX to I. VF is set to 1 when there is a range overflow (I+VX > 0xFFF),
        //         and 0 otherwise
        res = V[instr.vx] + I;
        V[0xF] = res > 0xFFF ? 1 : 0;
        I = res & 0xFFF;
        break;
    case 0x29:
        // FX29 -- Sets I to the location of the sprite for the character in VX. Characters
        //         0-F are represented by a 4x5 font
        std::cerr << "Warning: No sprite instructions implemented!\n";
        break;
    case 0x33:
        // FX33 -- take the decimal representation of VX, place the hundreds digit in memory
        //         at location in I, the tens digit at location I+1, and the ones digit at 
        //         location I+2
        {
            uint8_t hunds = V[instr.vx] / 100;
            uint8_t tens = (V[instr.vx] % 100) / 10;
            uint8_t ones = (V[instr.vx] % 100) % 10;
            mem.write(hunds, I);
            mem.write(tens, I+1);
            mem.write(ones, I+2);
        }
        break;
    case 0x55:
        // FX55 -- Store V0 to VX (inclusive) in mem starting at addr I.
        for (int i = 0; i <= instr.vx; i++) {
            mem.write(V[i], I+i);
        }
        I = I + instr.vx + 1;
        break;
    case 0x65:
        // FX65 -- Fill V0 to VX (inclusive) in mem starting at addr I.    
        for (int i = 0; i <= instr.vx; i++) {
            V[i] = mem.read(I+i);
        }
        I = I + instr.vx + 1;
        break;
    default:
        std::cerr << "Error (opF): unknown instruction!\n";
        std::exit(1);
    }
    pc += 2;
}

void Chip8::dump()
{
    mem.dump();

    std::ofstream ofile;
    ofile.open("chip8-regdump.txt");
    if (!ofile.is_open()) {
        std::cerr << "Failed to create regdump file!\n";
        return;
    }

    char str[16];
    sprintf(str, "0x%04X", I);
    ofile << "I: " << str << std::endl;
    sprintf(str, "0x%04X", pc);
    ofile << "pc: " << str << std::endl;
    // dump regs
    for (int i = 0; i < 16; i++) {
        ofile << "V" << i << ": " << (uint) V[i] << std::endl;
    }

    ofile.close();
}