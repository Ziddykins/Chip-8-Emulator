typedef struct chip8 {
    unsigned short op;            //opcode
    unsigned char  mem[4096];     //4K memory
    unsigned char  V[16];         //16 CPU registers, V0->VF
    unsigned short I;             //Index register
    unsigned short pc;            //Program counter
    unsigned char  gfx[64*32];    //64x32 graphics system

    unsigned char delay_timer;    //Counts down until zero at 60Hz
    unsigned char sound_timer;    //Sound plays when reaching zero

    unsigned short stack[16];     //16-level stack
    unsigned short stk_ptr;       //Stack pointer

    unsigned char key[16];        //Hex-based keypad

    unsigned int draw;            //Draw flag
} chip8;
