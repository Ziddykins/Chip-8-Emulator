#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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
    for (i=0; i<4096; i++) {
        if (i % 15 == 0) printf("\n");
        printf("%#04x ", Chip8.mem[i]);
    }
}

int load_rom (char *filename) {
    char *source = NULL;
    FILE *file = fopen(filename, "rb");
    int i;

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
    Chip8.op = Chip8.mem[Chip8.pc] << 8 | Chip8.mem[Chip8.pc + 1];
    printf("Opcode received: %#04x\n", Chip8.op);
    switch (Chip8.op & 0xF000) {
        case 0x2000: //Jump to subroutine at NNN
            Chip8.stack[Chip8.stk_ptr] = Chip8.pc;
            Chip8.stk_ptr++;
            printf("Jumping from %#04x to %#04x\n", Chip8.pc, Chip8.op & 0x0FFF);
            Chip8.pc = Chip8.op & 0x0FFF;
            break;
        case 0x6000: //Sets VX to NN
            Chip8.V[(Chip8.op & 0x0F00) >> 8] = Chip8.op & 0x00FF;
            printf("Set V[%d] to %#04x\n", (Chip8.op & 0x0F00) >> 8, Chip8.op & 0x00FF);
            Chip8.pc += 2;
            printf("Moved ahead to: %#04x\n", Chip8.pc);
            break;
        case 0xA000: //Sets index to NNN
            Chip8.I = Chip8.op & 0x0FFF;
            printf("Index set to %#04x\n", Chip8.op & 0x0FFF);
            Chip8.pc += 2;
            printf("Moved ahead to: %#04x\n", Chip8.pc);
            break;
        default:
            printf("I don't know what to do with opcode: %#04x, teach me!\n", Chip8.op);
            return 1;
    }
}

int main (int argc, char **argv) {
    initialize();
    if (load_rom(argv[1]) != 1) {
        while (1) {
            if (cycle() == 1) break;
        }
    }
    return 0;
}
