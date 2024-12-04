#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define WIDTH 64
#define HEIGHT 32
#define SCALE 20
#define MEMORY_SIZE 4096

void Init(unsigned char *ram, unsigned char *display, unsigned char *registers, unsigned short *PC, unsigned short *I, unsigned short *stack,
          unsigned int *delay_timer, unsigned int *sound_timer, unsigned char *sp, 
          unsigned short *opcode, unsigned char *font);

bool LoadRom(unsigned char *ram);

void Fetch(unsigned char *ram, unsigned short *opcode, unsigned short *PC);

void Decode(unsigned short opcode, unsigned short *PC, unsigned char *registers, unsigned short *I);

int main(int argc, char *argv[])
{
    unsigned char ram[MEMORY_SIZE];
    unsigned char registers[16];
    unsigned char display[WIDTH * HEIGHT] = {0};
    unsigned short stack[16];
    unsigned short PC;
    unsigned short I;
    unsigned char sp;
    unsigned int delay_timer;
    unsigned int sound_timer;
    unsigned short opcode;

    unsigned char font[80] = {
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

    Init(ram, display, registers, &PC, &I, stack, &delay_timer, &sound_timer, &sp, &opcode, font);

    ram[PC] = 0xAB;
    ram[PC+1] = 0xCD;

    Fetch(ram, &opcode, &PC);

    printf("opcode : 0x%hX\n", opcode);

    Decode(opcode, &PC, registers, &I);

    return 0;
}

void Init(unsigned char *ram, unsigned char *display, unsigned char *registers, unsigned short *PC, unsigned short *I, unsigned short *stack,
          unsigned int *delay_timer, unsigned int *sound_timer, unsigned char *sp, 
          unsigned short *opcode, unsigned char *font)
{

    for (int i = 0; i < MEMORY_SIZE; i++)
    {
        ram[i] = 0;
    }

    for (int i = 0; i < 16; i++)
    {
        registers[i] = 0;
    }

    for(int i = 0;i<WIDTH*HEIGHT;i++){
        display[i] = 0;
    }

    for (int i = 0; i < 80; i++)
    {
        ram[0x50 + i] = font[i];
    }

    *PC = 0x200;
    *I = 0;
    *sp = 0;
    *delay_timer = 0;
    *sound_timer = 0;
    *opcode = 0;
}

bool LoadRom(unsigned char *ram)
{
    FILE *rom = fopen("./1-chip8-logo.ch8", "rb");

    if (rom == NULL)
    {
        printf("Impossible d'ouvrir la rom.");
        fflush(stdout);
        return false;
    }

    size_t rom_size = fread(ram + 0x200, 1, MEMORY_SIZE - 0x200, rom);

    if (rom_size == 0)
    {
        printf("Erreur dans la lecture de la rom.");
        fflush(stdout);
        fclose(rom);
        return false;
    }

    fclose(rom);
    return true;
}

void Fetch(unsigned char *ram , unsigned short *opcode, unsigned short *PC){
    //1 part of ram is 1/2 instruction => 0xAB == 10101011 (There are multiple 0 before the binary code)
    //In order to combine these, we need to move the 0xAB on the left with << and then combine 
    //with the second half
    *opcode = (ram[*PC] << 8) | ram[*PC + 1];

    *PC += 2;
}

void Decode(unsigned short opcode, unsigned short *PC, unsigned char *registers, unsigned short *I){
    unsigned short firstNibble = (opcode & 0xF000) >> 12;
    unsigned short X = (opcode & 0x0F00) >> 8;
    unsigned short Y = (opcode & 0x00F0) >> 4;
    unsigned short N = (opcode & 0x000F);
    unsigned short NN = (opcode & 0x00FF);
    unsigned short NNN = (opcode & 0xFFF);

    switch(firstNibble){
        case 0x0:
            break;
        case 0x1:
            *PC = NNN;
            break;
        case 0x6:
            registers[X] = NN;
            break;
        case 0x7:
            registers[X] += NN;
            break;
        case 0xA:
            *I = NNN;
            break;
        case 0xD:
            break;
    }

    printf("NN : 0x%hX", NN);    
}