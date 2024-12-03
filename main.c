#include <stdio.h>
#include <stdint.h>
#include <unistd.h>


#define WIDTH 64
#define HEIGHT 32
#define MEMORY_SIZE 4096
#define CLS 0x00E0
#define JMP 0x1000
#define SET_RG 0x6
#define ADD_VAL 0x7
#define SET_IND 0xA
#define DSPL 0xD

unsigned char ram[MEMORY_SIZE];
unsigned char registers[16];
//PC indicates where is the instruction
unsigned short PC;
unsigned short I;
unsigned short stack;
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

void Init(){

    for(int i=0;i<MEMORY_SIZE;i++){
        ram[i] = 0;
    }

    for(int i = 0;i<16;i++){
        registers[i] = 0;
    }

    PC = 0x200;
    I = 0;
    stack = 0;
    delay_timer = 0;
    sound_timer = 0;



    printf("Fin de l'initialisation.");
}

void Fetch(){
    opcode = ram[PC] << 8 | ram[PC+1];

    PC+=2;
}

void Decode(){
    // >> 8 to put the nibble at the end
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t N = (opcode & 0x000F);
    uint8_t NN = (opcode & 0x00FF);
    uint8_t NNN = (opcode & 0x0FFF);

    switch(opcode & 0xF0)
    {
        case 0x0000:
            ClearScreen();
            break;      
        case 0x1000:
            ram[PC] = NNN;
            break;
        case 0x6000:
            registers[X] = NN;
            break;
        case 0x7000:
            registers[X] += NN;
            break;
        case 0xA000:
            I = NNN;
            break;
        case 0xD000:
            DrawScreen();
            break;
        default:
            printf("Incorrect instruction.");
    }
}

void DrawScreen(){
    printf("Draw Screen");
}

void ClearScreen(){
    printf("Screen Cleared");
}

void DecrementTimer(unsigned int *timer){
    while ((*timer) > 0)
    {
        (*timer)--;
        usleep(16667);
    }
}

int main(int argc, char *argv[])
{
    int test;
    Init();

    while (1==1)
    {
        
    }
    
    return 0;
}