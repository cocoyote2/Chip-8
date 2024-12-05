#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define WIDTH 64
#define HEIGHT 32
#define SCALE 20
#define MEMORY_SIZE 4096

void Init(unsigned char *ram, unsigned char *display, unsigned char *registers, unsigned short *PC, unsigned short *I, unsigned short *stack,
          unsigned int *delay_timer, unsigned int *sound_timer, unsigned char *sp,
          unsigned short *opcode, unsigned char *font);

bool LoadRom(unsigned char *ram);

void Fetch(unsigned char *ram, unsigned short *opcode, unsigned short *PC);

void Decode(unsigned short opcode, unsigned short *PC, unsigned char *registers, unsigned short *I, unsigned char *display, SDL_Renderer *renderer);

int InitSDL(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture);

void ClearScreen(unsigned char *display, SDL_Renderer *renderer);

int HandleEvents(SDL_Event event);

void DisplaySprite(unsigned char *registers, unsigned char VX, unsigned char VY, unsigned short N);

void DisplaySDL();

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
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool quit = false;

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

    if(InitSDL(window, renderer, texture) == 1){
        return EXIT_FAILURE;
    }

    ram[PC] = 0xAB;
    ram[PC + 1] = 0xCD;

    while(!quit){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(HandleEvents(event) == 0){
                return EXIT_SUCCESS;
            }

            Fetch(ram, &opcode, &PC);

            printf("opcode : 0x%hX\n", opcode);

            Decode(opcode, &PC, registers, &I, display, renderer);

            SDL_Delay(16);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
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

    for (int i = 0; i < WIDTH * HEIGHT; i++)
    {
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

int InitSDL(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0){
        fprintf(stderr, "Couldn't initialize SDL : %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH*SCALE, HEIGHT*SCALE, SDL_WINDOW_SHOWN);

    if(window == NULL){
        fprintf(stderr, "Couldn't create a window : %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if(renderer == NULL){
        fprintf(stderr, "Couldn't initialize renderer : %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH*SCALE, HEIGHT*SCALE);

    if(texture == NULL){
        fprintf(stderr, "Couldn't initialize the texure : %s", SDL_GetError());
    }

    return EXIT_SUCCESS;
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

void Fetch(unsigned char *ram, unsigned short *opcode, unsigned short *PC)
{
    // 1 part of ram is 1/2 instruction => 0xAB == 10101011 (There are multiple 0 before the binary code)
    // In order to combine these, we need to move the 0xAB on the left with << and then combine
    // with the second half
    *opcode = (ram[*PC] << 8) | ram[*PC + 1];

    *PC += 2;
}

void Decode(unsigned short opcode, unsigned short *PC, unsigned char *registers, unsigned short *I, unsigned char *display, SDL_Renderer *renderer)
{
    unsigned short firstNibble = (opcode & 0xF000) >> 12;
    unsigned short X = (opcode & 0x0F00) >> 8;
    unsigned short Y = (opcode & 0x00F0) >> 4;
    unsigned short N = (opcode & 0x000F);
    unsigned short NN = (opcode & 0x00FF);
    unsigned short NNN = (opcode & 0xFFF);

    switch (firstNibble)
    {
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
        ClearScreen(display, renderer);
        break;
    }

    printf("NN : 0x%hX", NN);
}

void ClearScreen(unsigned char *display, SDL_Renderer *renderer)
{
    for (int i = 0; i < WIDTH * HEIGHT; i++)
    {
        display[i] = 0;
    }

    SDL_RenderClear(renderer);
}

int HandleEvents(SDL_Event event){
    switch (event.type)
    {
    case SDL_QUIT:
        return EXIT_SUCCESS;
        break;
    
    default:
        break;
    }
}

void DisplaySprite(unsigned char *registers, unsigned char VX, unsigned char VY, unsigned short N){
    unsigned char x = registers[VX]%64;
    unsigned char y = registers[VY]%32;

    registers[0xF] = 0;
}

void DisplaySDL(){

}