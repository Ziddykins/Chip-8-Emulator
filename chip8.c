#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include "chip8.h"

chip8 Chip8;

unsigned char fontset[80] = { 
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void initialize (void) {
    int i;
    Chip8.pc      = 0x200;
    Chip8.op      = 0;
    Chip8.I       = 0;
    Chip8.stk_ptr = 0;

    for (i=0; i<64*32; i++)
        Chip8.gfx[i] = 0;

    for (i=0; i<16; i++) {
        Chip8.stack[i] = 0;
        Chip8.V[i] = 0;
    }

    for (i=0; i<4096; i++)
        Chip8.mem[i] = 0;

    for (i=0; i<80; i++)
        Chip8.mem[i] = fontset[i];

    Chip8.delay_timer = 0;
    Chip8.sound_timer = 0;
}

void read_mem (void) {
    int i;
    printf("Mem:\n");
    for (i=512; i<1296; i++) {
        if (i % 25 == 0) printf("\n");
        printf("%#04X ", Chip8.mem[i]);
    }
    printf("\n");
    for (i=0; i<16; i++) {
        if (i == 8)  printf("\n");
        if (i != 15) printf("R%d: %#04X - ", i, Chip8.V[i]);
        else         printf("R%d: %#04X\n", i, Chip8.V[i]);
    }
    for (i=0; i<16; i++) {
        printf("stack[%d]: %#04x ptr: %#04x\n", i, Chip8.stack[i], Chip8.stk_ptr);
    }
        
    printf("Index: %#04X\n", Chip8.I);
    usleep(50000);
}

int load_rom (char *filename) {
    char *source = NULL;
    FILE *file = fopen(filename, "rb");
    size_t i;

    if (file != NULL) {
        if (fseek(file, 0L, SEEK_END) == 0) {
            long buf_size = ftell(file);
            if (buf_size == -1) {
                printf("Error reading file size\n");
                return 1;
            }

            source = malloc(sizeof(char) * (buf_size));
            assert(source != NULL);
            fseek(file, 0L, SEEK_SET);

            size_t length = fread(source, sizeof(char), buf_size, file);

            if (length == 0) {
                printf("Error reading file\n");
                return 1;
            }

            for (i=0; i<=length; i++)
                Chip8.mem[i+512] = source[i];
        }
    }
    fclose(file);
    free(source);
    return 0;
}

int cycle (void) {
    int i;
    Chip8.op = Chip8.mem[Chip8.pc] << 8 | Chip8.mem[Chip8.pc + 1];
    printf("Opcode received: %#04x\n", Chip8.op);
    switch (Chip8.op & 0xF000) {
        case 0x0000:
            switch (Chip8.op & 0x000F) {
                case 0x0000:
                    for (i=0; i<2048; i++)
                        Chip8.gfx[i] = 0x0;
                    Chip8.pc += 2;
                    printf("Cleared screen\nMoved ahead to %#04x\n", Chip8.pc);
                    break;
                case 0x000E: //Returns from a subroutine
                    Chip8.stk_ptr--;
                    Chip8.pc = Chip8.stack[Chip8.stk_ptr];
                    Chip8.pc += 2;
                    printf("Returned from subroutine, now at %#04x\n", Chip8.pc);
                    break;
                default:
                    printf("I don't know opcode %#04x, teach me!\n", Chip8.op);
                    return 1;
            }
            break;
        case 0x1000: //Jumps to address NNN
            Chip8.pc = Chip8.op & 0x0FFF;
            printf("Jumped to %#04x\n", Chip8.op & 0x0FFF);
            break;
        case 0x2000: //Jump to subroutine at NNN
            Chip8.stack[Chip8.stk_ptr] = Chip8.pc;
            Chip8.stk_ptr++;
            printf("Jumping from %#04x to %#04x\n", Chip8.pc, Chip8.op & 0x0FFF);
            Chip8.pc = Chip8.op & 0x0FFF;
            break;
       case 0x3000: //Skips the next instruction if VX == NN
            if (Chip8.V[(Chip8.op & 0x0F00) >> 8] == (Chip8.op & 0x00FF)) {
                Chip8.pc += 4;
                printf("V[%d] == %#04x, moving ahead 4\n", (Chip8.op & 0x0F00) >> 8, Chip8.op & 0x00FF);
            } else {
                Chip8.pc += 2;
                printf("Moving ahead to %#04x\n", Chip8.pc);
            }
            break;
        case 0x4000: //Skips the next instruction if VX != NN
            if (Chip8.V[(Chip8.op & 0x0F00) >> 8] != (Chip8.op & 0x00FF)) {
                Chip8.pc += 4;
                printf("V[%d] != %#04x, moving ahead 4\n", (Chip8.op & 0x0F00) >> 8, Chip8.op & 0x00FF);
            } else {
                Chip8.pc += 2;
                printf("Moving ahead to %#04x\n", Chip8.pc);
            }
            break;
        case 0x5000: //Skips the next instruction if VX == VY
            if (Chip8.V[(Chip8.op & 0x0F00) >> 8] == (Chip8.op & 0x00F0) >> 4) {
                Chip8.pc += 4;
                printf("V[%d] == V[%d], moving ahead 4\n", (Chip8.op & 0x0F00) >> 8, (Chip8.op & 0x00F0) >> 4);
            } else {
                Chip8.pc += 2;
                printf("Moving ahead to %#04x\n", Chip8.pc);
            }
            break;
        case 0x6000: //Sets VX to NN
            Chip8.V[(Chip8.op & 0x0F00) >> 8] = Chip8.op & 0x00FF;
            printf("Set V[%d] to %#04x\n", (Chip8.op & 0x0F00) >> 8, Chip8.op & 0x00FF);
            Chip8.pc += 2;
            printf("Moved ahead to: %#04x\n", Chip8.pc);
            break;
        case 0x7000: //Adds NN to VX
            Chip8.V[(Chip8.op & 0x0F00) >> 8] += Chip8.op & 0x00FF;
            Chip8.pc += 2;
            printf("Added %#04x to V[%d]\n", Chip8.op & 0x00FF, (Chip8.op & 0x0F00) >> 8); 
            printf("Moved ahead to: %#04x\n", Chip8.pc);
            break;
        case 0x8000: //Register stuffs
            switch (Chip8.op & 0x000F) {
                case 0x0000: //Sets VX to the value of VY
                    Chip8.V[(Chip8.op & 0x0F00) >> 8] = 
                    Chip8.V[(Chip8.op & 0x00F0) >> 4];
                    printf("V[%d] set to V[%d]\nMoved ahead to %#04x\n", (Chip8.op & 0x0F00) >> 8, (Chip8.op & 0x00F0) >> 4, Chip8.pc + 2);
                    Chip8.pc += 2;
                    break;
                case 0x0001: //Sets VX to VX | VY
                    Chip8.V[(Chip8.op & 0x0F00) >> 8] |=
                    Chip8.V[(Chip8.op & 0x00F0) >> 4];
                    printf("V[%d] or'd to V[%d]\nMoved ahead to %#04x\n", (Chip8.op & 0x0F00) >> 8, (Chip8.op & 0x00F0) >> 4, Chip8.pc + 2);
                    Chip8.pc += 2;
                    break;
                case 0x0002: //Sets VX to VX & VY
                    Chip8.V[(Chip8.op & 0x0F00) >> 8] &=
                    Chip8.V[(Chip8.op & 0x00F0) >> 4];
                    printf("V[%d] and'd to V[%d]\nMoved ahead to %#04x\n", (Chip8.op & 0x0F00) >> 8, (Chip8.op & 0x00F0) >> 4, Chip8.pc + 2);
                    Chip8.pc += 2;
                    break;
                case 0x0003: //Sets VX to VX ^ VY
                    Chip8.V[(Chip8.op & 0x0F00) >> 8] ^=
                    Chip8.V[(Chip8.op & 0x00F0) >> 4];
                    printf("V[%d] xor'd to V[%d]\nMoved ahead to %#04x\n", (Chip8.op & 0x0F00) >> 8, (Chip8.op & 0x00F0) >> 4, Chip8.pc + 2);
                    Chip8.pc += 2;
                    break;
                case 0x0004: //Adds VY to VX - VF is set to 1 when there's a
                             //carry and to 0 when there isn't
                    if (Chip8.V[(Chip8.op & 0x00F0) >> 4] > 
                                  (0xFF - (Chip8.V[(Chip8.op & 0x0F00) >> 8]))) {
                        Chip8.V[0xF] = 1;
                        printf("Carry flag set\n");
                    } else {
                        Chip8.V[0xF] = 0;
                    }
                    Chip8.V[(Chip8.op & 0x00F0) >> 4] +=
                    Chip8.V[(Chip8.op & 0x0F00) >> 8];
                    printf("Added V[%d] to V[%d]\n", Chip8.V[(Chip8.op & 0x00F0) >> 4],  Chip8.V[(Chip8.op & 0x0F00) >> 8]);
                    Chip8.pc += 2;
                    printf("Moved ahead to %#04x\n", Chip8.pc);
                    break;
                case 0x0005: //VY is subtracted from VX. VF is set to 0 when 
                             //there's a borrow, and 1 when there isn't.
                    if (Chip8.V[(Chip8.op & 0x00F0) >> 4] > 
                                (0xFF - (Chip8.V[(Chip8.op & 0x0F00) >> 8]))) {
                        Chip8.V[0xF] = 0;
                        printf("Borrow flag set\n");
                    } else {
                        Chip8.V[0xF] = 1;
                    }
                    Chip8.V[(Chip8.op & 0x00F0) >> 4] -=
                    Chip8.V[(Chip8.op & 0x0F00) >> 8];
                    printf("subtracted V[%d] to V[%d]\n", Chip8.V[(Chip8.op & 0x00F0) >> 4],  Chip8.V[(Chip8.op & 0x0F00) >> 8]);
                    Chip8.pc += 2;
                    printf("Moved ahead to %#04x\n", Chip8.pc);
                    break;
                case 0x0006: //VF set to LSB, shift VX to the right by one
                    Chip8.V[0xF] = Chip8.V[(Chip8.op & 0x0F00) >> 8] & 0x1;
                    Chip8.V[(Chip8.op & 0x0F00) >> 8] >>= 1;
                    Chip8.pc += 2;
                    printf("Register V[%d] shifted right by one\n", (Chip8.op & 0x0F00) >> 8);
                    break;
                case 0x0007: //Set VX to VY minus VX. 
                             //VF set to 0 when there's a borrow
                    if (Chip8.V[(Chip8.op & 0x0F00) >> 8] >
                                Chip8.V[(Chip8.op & 0x00F0) >> 4]) {
                        Chip8.V[0xF] = 0;
                    } else {
                        Chip8.V[0xF] = 1;
                    }
                    Chip8.V[(Chip8.op & 0x0F00) >> 8] =
                    Chip8.V[(Chip8.op & 0x0F00) >> 4] -
                    Chip8.V[(Chip8.op & 0x0F00) >> 8];
                    Chip8.pc += 2;
                    printf("Set V[%d] to V[%d] - V[%d]\n", Chip8.V[(Chip8.op & 0x0F00) >> 8], Chip8.V[(Chip8.op & 0x0F00) >> 4], Chip8.V[(Chip8.op & 0x0F00) >> 8]);
                    printf("Moved ahead to %#04x\n", Chip8.pc);
                    break;
                case 0x000E: //Shift VX left by one. VF set to MSB before shift
                    Chip8.V[0xF] = Chip8.V[(Chip8.op & 0x0F00) >> 8] >> 7;
                    Chip8.V[(Chip8.op & 0x0F00) >> 8] <<= 1;
                    Chip8.pc += 2;
                    printf("Shifted V[%d] left by one\nMoved ahead to %#04x\n", (Chip8.op & 0x0F00) >> 8, Chip8.pc);
                    break;
                default:
                    printf("I don't know opcode %#04x, teach me!\n", Chip8.op);
                    return 1;
            }
        case 0xA000: //Sets index to NNN
            Chip8.I = Chip8.op & 0x0FFF;
            printf("Index set to %#04x\n", Chip8.op & 0x0FFF);
            Chip8.pc += 2;
            printf("Moved ahead to: %#04x\n", Chip8.pc);
            break;
        case 0xD000:
        { //Draws sprites. If a pixel is flipped VX = 1
            unsigned short x = Chip8.op & 0x0F00 >> 8;
            unsigned short y = Chip8.op & 0x00F0 >> 4;
            unsigned short h = Chip8.op & 0x000F;
            unsigned short px;
            int xl, yl;

            Chip8.V[0xF] = 0;

            for (yl=0; yl<h; yl++) {
                px = Chip8.mem[Chip8.I + yl];
                for (xl=0; xl<8; xl++) {
                    if ((px & (0x80 >> xl)) != 0) {
                        if (Chip8.gfx[(x+xl+((y+yl)*64))] == 1) {
                            Chip8.V[0xF] = 1;
                            printf("Collision detection\n");
                        }
                        Chip8.gfx[x+xl+((y+yl)*64)] ^= 1;
                    }
                }
            }
            Chip8.draw = 1;
            Chip8.pc  += 2;
            printf("Moved ahead to: %#04x\n", Chip8.pc);
            break;
        }
        default:
            printf("I don't know what to do with opcode: %#04x, teach me!\n", Chip8.op);
            return 1;
    }

    if (Chip8.delay_timer > 0) --Chip8.delay_timer;

    if (Chip8.sound_timer > 0) {
        if (Chip8.sound_timer == 1)
            printf("BEEP\n");
        --Chip8.sound_timer;
    }
    return 0;
}

void render(void) {
    for (int y=0; y<32; ++y) {
        for (int x=0; x<64; ++x) {
            if   (Chip8.gfx[(y*64) + x] == 0) printf("O");
            else printf(" ");
        }
        printf("\n");
    }
    printf("\n");
}

int main (int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s example.c8\n", argv[0]);
        return 1;
    }

    initialize();

    if (load_rom(argv[1]) != 1) {
        while (1) {
            int last_time = 0;
            int current_time = SDL_GetTicks();
            if (current_time > last_time + 60) {
                if (cycle() == 1) break;
//                read_mem();
//  render();              
                usleep(500000);
                current_time = last_time;
            }
        }
    }
    return 0;
}
