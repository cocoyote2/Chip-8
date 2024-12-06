#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 64
#define HEIGHT 32
#define SCALE 20
#define MEMORY_SIZE 4096

void Init(unsigned char *ram, unsigned char *display, unsigned char *registers, unsigned short *PC, unsigned short *I, unsigned short *stack,
          unsigned int *delay_timer, unsigned int *sound_timer, unsigned char *stackPointer,
          unsigned short *opcode, unsigned char *font);

bool LoadRom(unsigned char *ram);

void Fetch(unsigned char *ram, unsigned short *opcode, unsigned short *PC);

void Decode(unsigned short opcode, unsigned short *PC, unsigned char *registers, unsigned short *I,
            unsigned char *display, SDL_Renderer **renderer, unsigned char *ram, SDL_Texture **texture, int pitch,
            unsigned short *stack, unsigned char *stackPointer);

int InitSDL(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture);

void ClearScreen(unsigned char *display, SDL_Renderer **renderer);

int HandleEvents(SDL_Event event, bool *quit);

void DisplaySprite(unsigned char *registers, unsigned char *ram, unsigned char VX, unsigned char VY,
                   unsigned short N, unsigned short I, unsigned char *display);

void DisplaySDL(unsigned char *display, SDL_Texture **texture, SDL_Renderer **renderer, int pitch);

void DecrementTimers(unsigned int *delay_timer, unsigned int *sound_timer);

void Add(unsigned char *registers, unsigned short X, unsigned short Y);

int main(int argc, char *argv[])
{
    unsigned char ram[MEMORY_SIZE];
    unsigned char registers[16];
    unsigned char display[WIDTH * HEIGHT] = {0};
    unsigned short stack[16];
    unsigned short PC;
    unsigned short I;
    unsigned char stackPointer;
    unsigned int delay_timer;
    unsigned int sound_timer;
    unsigned short opcode;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool quit = false;
    srand(time(NULL));

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

    Init(ram, display, registers, &PC, &I, stack, &delay_timer, &sound_timer, &stackPointer, &opcode, font);

    if(InitSDL(&window, &renderer, &texture) == 1){
        return EXIT_FAILURE;
    }

    int videoPitch = sizeof(display[0]) * WIDTH;

    if(!LoadRom(ram)){
        return EXIT_FAILURE;
    }

    while(!quit){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(HandleEvents(event, &quit) == 0){
                return EXIT_SUCCESS;
            }

            Fetch(ram, &opcode, &PC);

            Decode(opcode, &PC, registers, &I, display, &renderer, ram, &texture, videoPitch, stack, &stackPointer);

            DecrementTimers(&delay_timer, &sound_timer);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void Init(unsigned char *ram, unsigned char *display, unsigned char *registers, unsigned short *PC, unsigned short *I, unsigned short *stack,
          unsigned int *delay_timer, unsigned int *sound_timer, unsigned char *stackPointer,
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

    for(int i = 0;i<16;i++){
        stack[i] = 0;
    }

    *PC = 0x200;
    *I = 0;
    *stackPointer = 0;
    *delay_timer = 0;
    *sound_timer = 0;
    *opcode = 0;
}

int InitSDL(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0){
        fprintf(stderr, "Couldn't initialize SDL : %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    *window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH*SCALE, HEIGHT*SCALE, SDL_WINDOW_SHOWN);

    if(*window == NULL){
        fprintf(stderr, "Couldn't create a window : %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);

    if(*renderer == NULL){
        fprintf(stderr, "Couldn't initialize renderer : %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    if(*texture == NULL){
        fprintf(stderr, "Couldn't initialize the texure : %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Initialization successfull !");
    return EXIT_SUCCESS;
}

bool LoadRom(unsigned char *ram)
{
    FILE *rom = fopen("./roms/Minimalgame[RevivalStudios,2007].ch8", "rb");

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

void Decode(unsigned short opcode, unsigned short *PC, unsigned char *registers, unsigned short *I, 
            unsigned char *display, SDL_Renderer **renderer, unsigned char *ram, SDL_Texture **texture, int pitch,
            unsigned short *stack, unsigned char *stackPointer)
{
    unsigned short firstNibble = (opcode & 0xF000) >> 12;
    unsigned short X = (opcode & 0x0F00) >> 8;
    unsigned short Y = (opcode & 0x00F0) >> 4;
    unsigned short N = (opcode & 0x000F);
    unsigned short NN = (opcode & 0x00FF);
    unsigned short NNN = (opcode & 0xFFF);
    unsigned short shiftedBit;
    unsigned short randomNumber;

    switch (firstNibble)
    {
        case 0x0:
            if(opcode == 0x00E0){
                ClearScreen(display, renderer);
            }else if (opcode == 0x00EE){
                *PC = stack[*stackPointer];
                stack[*stackPointer] = 0;
                *stackPointer--;
            }
            break;
        case 0x1:
            *PC = NNN;
            break;
        case 0x2:
            stack[*stackPointer] = *PC;
            *stackPointer++;
            break;
        case 0x3:
            if(registers[X] == NN){
                *PC += 2;
            }
            break;
        case 0x4:
            if(registers[X] != NN){
                *PC += 2;
            }
            break;
        case 0x5:
            if(N == 0 && registers[X] == registers[Y]){
                *PC +=2;
            }
            break;
        case 0x6:
            registers[X] = NN;
            break;
        case 0x7:
            registers[X] += NN;
            break;
        case 0x9:
            if(N==0 & registers[X] != registers[Y]){
                *PC += 2;
            }
            break;
        case 0x8:
            switch(N){
                case 0:
                    registers[X] = registers[Y];
                    break;
                case 1:
                    registers[X] = registers[X] | registers[Y];
                    break;
                case 2:
                    registers[X] = registers[X] & registers[Y];
                    break;
                case 3:
                    registers[X] = registers[X] ^ registers[Y];
                    break;
                case 4:
                    Add(registers, X, Y);
                    break;
                case 5:
                    registers[X] = (registers[X] - registers[Y]) % 256;
                    registers[0xF] = (registers[X] > registers[Y]) ? 1 : 0;
                    break;
                case 6:
                    //Not configurable for the moment
                    //registers[X] = registers[Y];
                    shiftedBit = registers[X] & 0x01;
                    registers[0xF] = shiftedBit;
                    registers[X] >> 1;
                    break;
                case 7:
                    registers[X] = (registers[X] - registers[Y]) % 256;
                    registers[0xF] = (registers[Y] > registers[X]) ? 1 : 0;
                    break;
                default:
                    shiftedBit = (registers[X] & 0x80) >> 7;
                    registers[0xF] = shiftedBit;
                    registers[X] << 1;
            }
            break;
        case 0xA:
            *I = NNN;
            break;
        case 0xB:
            *PC = NNN + registers[0];
            break;
        case 0xC:
            randomNumber = rand()%256;
            registers[X] = randomNumber & NN;
            break;
        case 0xD:
            DisplaySprite(registers, ram, X, Y, N, *I, display);
            DisplaySDL(display, texture, renderer, pitch);
            break;
        case 0xE:
            switch(opcode & 0xF0FF){
                case 0xE09E:
                    break;
                case 0xE0A1:
                    break;
            }
            break;
    }
}

void ClearScreen(unsigned char *display, SDL_Renderer **renderer)
{
    for (int i = 0; i < WIDTH * HEIGHT; i++)
    {
        display[i] = 0;
    }

    if(SDL_RenderClear(*renderer) < 0){
        fprintf(stderr, "Error when clearing renderer : %s", SDL_GetError());
        return;
    }

    SDL_RenderPresent(*renderer);
}

int HandleEvents(SDL_Event event, bool *quit){
    switch (event.type)
    {
    case SDL_QUIT:
        *quit = true;
        break;
    
    default:
        break;
    }
}

void DisplaySprite(unsigned char *registers, unsigned char *ram, unsigned char VX, unsigned char VY, 
                unsigned short N, unsigned short I, unsigned char *display){
    unsigned char x = registers[VX] % WIDTH;
    unsigned char y = registers[VY] % HEIGHT;

    registers[0xF] = 0;

    for(int row = 0;row < N;row++){
        unsigned char currByte = ram[I + row];

        for(int col = 0;col<8;col++){
            unsigned char spritePixel = currByte & (0x80u >> col);

            unsigned int pixelIndex = (y+row) * WIDTH + (x+col);

            if((x+col) >= WIDTH || (y+row) >= HEIGHT)
                continue;

            unsigned char currPixel = display[(y + row) * WIDTH + (x + col)];

            if(spritePixel){
                if (currPixel == 0xFF){
                    registers[0xF] = 1;
                }

                display[pixelIndex] ^= 0xFF;
            }
        }
    }
}

void DisplaySDL(unsigned char *display, SDL_Texture **texture, SDL_Renderer **renderer, int pitch)
{
    void *pixels = NULL;
    int pitch_in_bytes = 0;

    if (SDL_LockTexture(*texture, NULL, &pixels, &pitch_in_bytes) != 0)
    {
        fprintf(stderr, "Unable to lock texture: %s\n", SDL_GetError());
        return;
    }

    uint32_t *pixel_ptr = (uint32_t *)pixels;

    for (int i = 0; i < WIDTH * HEIGHT; ++i)
    {
        pixel_ptr[i] = (display[i] == 0xFF) ? 0xFFFFFFFF : 0x00000000;
    }

    SDL_UnlockTexture(*texture);

    SDL_RenderClear(*renderer);
    SDL_RenderCopy(*renderer, *texture, NULL, NULL);
    SDL_RenderPresent(*renderer);
}

void DecrementTimers(unsigned int *delay_timer, unsigned int *sound_timer){
    if(*delay_timer > 0){
        *delay_timer--;
    }

    if(*sound_timer > 0){
        *sound_timer--;
        Beep(750, 8);
    }

    SDL_Delay(16);
}

void Add(unsigned char *registers, unsigned short X, unsigned short Y){
    unsigned int res = registers[X] + registers[Y];
    registers[X] = res & 0xFF;

    registers[0xF] = (res > 255) ? 1 : 0;
}

// TODO: handleKeys function.