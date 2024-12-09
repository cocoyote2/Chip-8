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
#define CHIP8_KEY_COUNT 16

void Init(uint8_t *ram, uint8_t *display, uint8_t *registers, uint16_t *PC, uint16_t *I, uint16_t *stack,
          uint32_t *delay_timer, uint32_t *sound_timer, uint8_t *stackPointer,
          uint16_t *opcode, uint8_t *font);

bool LoadRom(uint8_t *ram);

void Fetch(uint8_t *ram, uint16_t *opcode, uint16_t *PC);

void Decode(uint16_t opcode, uint16_t *PC, uint8_t *registers, uint16_t *I,
            uint8_t *display, SDL_Renderer **renderer, uint8_t *ram, SDL_Texture **texture, int pitch,
            uint16_t *stack, uint8_t *stackPointer, uint8_t *keypad, uint32_t *delay_timer, uint32_t *sound_timer);

int InitSDL(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture);

void ClearScreen(uint8_t *display, SDL_Renderer **renderer);

int HandleEvents(SDL_Event event, bool *quit);

void DisplaySprite(uint8_t *registers, uint8_t *ram, uint8_t VX, uint8_t VY,
                   uint16_t N, uint16_t I, uint8_t *display);

void DisplaySDL(uint8_t *display, SDL_Texture **texture, SDL_Renderer **renderer, int pitch);

void DecrementTimers(uint32_t *delay_timer, uint32_t *sound_timer);

void Add(uint8_t *registers, uint16_t X, uint16_t Y);

void handleKeys(SDL_Event event, SDL_Scancode *keymap, uint8_t *keypad);

bool isKeyPressed(uint8_t key, uint8_t *keypad);

void instruction0x0A(uint8_t *registers, uint16_t X, uint8_t *keypad);

void instruction0x29(uint16_t *I, uint16_t X, uint8_t *registers);

    int main(int argc, char *argv[])
{
    uint8_t ram[MEMORY_SIZE];
    uint8_t registers[16];
    uint8_t display[WIDTH * HEIGHT] = {0};
    uint16_t stack[16];
    uint16_t PC;
    uint16_t I;
    uint8_t stackPointer;
    uint32_t delay_timer;
    uint32_t sound_timer;
    uint16_t opcode;
    uint8_t keypad[CHIP8_KEY_COUNT];
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool quit = false;
    srand(time(NULL));

    uint8_t font[80] = {
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

    SDL_Scancode keymap[CHIP8_KEY_COUNT] = {
        SDL_SCANCODE_1, // 0x1 -> 1
        SDL_SCANCODE_2, // 0x2 -> 2
        SDL_SCANCODE_3, // 0x3 -> 3
        SDL_SCANCODE_4, // 0x4 -> 4
        SDL_SCANCODE_Q, // 0x5 -> Q
        SDL_SCANCODE_W, // 0x6 -> W
        SDL_SCANCODE_E, // 0x7 -> E
        SDL_SCANCODE_R, // 0x8 -> R
        SDL_SCANCODE_A, // 0x9 -> A
        SDL_SCANCODE_S, // 0xA -> S
        SDL_SCANCODE_D, // 0xB -> D
        SDL_SCANCODE_F, // 0xC -> F
        SDL_SCANCODE_Z, // 0xD -> Z
        SDL_SCANCODE_X, // 0xE -> X
        SDL_SCANCODE_C, // 0xF -> C
        SDL_SCANCODE_V  // 0x10 -> V 
    };

    Init(ram, display, registers, &PC, &I, stack, &delay_timer, &sound_timer, &stackPointer, &opcode, font);

    if(InitSDL(&window, &renderer, &texture) == 1){
        return EXIT_FAILURE;
    }

    int videoPitch = sizeof(display[0]) * WIDTH;

    if(!LoadRom(ram)){
        return EXIT_FAILURE;
    }

    uint32_t lastTime = SDL_GetTicks();
    while(!quit){
        SDL_Event event;
        uint32_t currTime = SDL_GetTicks();
        while(SDL_PollEvent(&event)){
            if(HandleEvents(event, &quit) == 0){
                return EXIT_SUCCESS;
            }

            handleKeys(event, keymap, keypad);
        }

        if (currTime - lastTime >= 2) { // 2 ms pour 500 Hz
            Fetch(ram, &opcode, &PC);

            Decode(opcode, &PC, registers, &I, display, &renderer, ram, &texture, videoPitch, stack, &stackPointer, keypad, &delay_timer, &sound_timer);
            lastTime = currTime;
        }

        // Mettre à jour les timers à 60 Hz
        if (currTime % 16 == 0) {
             DecrementTimers(&delay_timer, &sound_timer);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void Init(uint8_t *ram, uint8_t *display, uint8_t *registers, uint16_t *PC, uint16_t *I, uint16_t *stack,
          uint32_t *delay_timer, uint32_t *sound_timer, uint8_t *stackPointer,
          uint16_t *opcode, uint8_t *font)
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

bool LoadRom(uint8_t *ram)
{
    FILE *rom = fopen("./roms/4-flags.ch8", "rb");

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

void Fetch(uint8_t *ram, uint16_t *opcode, uint16_t *PC)
{
    // 1 part of ram is 1/2 instruction => 0xAB == 10101011 (There are multiple 0 before the binary code)
    // In order to combine these, we need to move the 0xAB on the left with << and then combine
    // with the second half
    *opcode = (ram[*PC] << 8) | ram[*PC + 1];

    *PC += 2;
}

void Decode(uint16_t opcode, uint16_t *PC, uint8_t *registers, uint16_t *I, 
            uint8_t *display, SDL_Renderer **renderer, uint8_t *ram, SDL_Texture **texture, int pitch,
            uint16_t *stack, uint8_t *stackPointer, uint8_t *keypad, uint32_t *delay_timer, uint32_t *sound_timer)
{
    uint16_t firstNibble = (opcode & 0xF000) >> 12;
    uint16_t X = (opcode & 0x0F00) >> 8;
    uint16_t Y = (opcode & 0x00F0) >> 4;
    uint16_t N = (opcode & 0x000F);
    uint16_t NN = (opcode & 0x00FF);
    uint16_t NNN = (opcode & 0xFFF);
    uint16_t shiftedBit;
    uint16_t randomNumber;

    switch (firstNibble)
    {
        case 0x0:
            if(opcode == 0x00E0){
                ClearScreen(display, renderer);
            }else if (opcode == 0x00EE){
                if(*stackPointer > 0){
                    (*stackPointer)--;
                    *PC = stack[*stackPointer];
                    stack[*stackPointer] = 0;
                }
            }
            break;
        case 0x1:
            *PC = NNN;
            break;
        case 0x2:
            if(*stackPointer < 16){
                stack[*stackPointer] = *PC;
                (*stackPointer)++;
            }
            *PC = NNN;
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
                    registers[X] -= registers[Y] & 0xFF;
                    registers[0xF] = (registers[X] >= registers[Y]) ? 1 : 0;               
                    break;
                case 6:
                    //Not configurable for the moment
                    //registers[X] = registers[Y];
                    shiftedBit = registers[X] & 0x01;
                    registers[X] >>= 1;
                    registers[0xF] = shiftedBit;
                    break;
                case 7:
                    registers[X] = (registers[Y] - registers[X]) & 0xFF;
                    registers[0xF] = (registers[Y] > registers[X]) ? 1 : 0;
                    break;
                case 0xE:
                    registers[0xF] = (registers[X] & 0x80) >> 7;
                    registers[X] <<= 1;
                    break;
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
                    if(X >= 0x0 && X <= 0xF && keypad[X] == 1){
                        *PC += 2;
                    }
                    break;
                case 0xE0A1:
                    if (X >= 0x0 && X <= 0xF && keypad[X] == 0)
                    {
                        *PC +=2;
                    }
                    break;
            }
            break;
        case 0xF:
            switch(NN){
                case 0x07:
                    registers[X] = *delay_timer;
                    break;
                case 0x15:
                    *delay_timer = registers[X];
                    break;
                case 0x18:
                    *sound_timer = registers[X];
                    break;
                case 0x1E:
                    *I = *I + registers[X];
                    break;
                case 0x0A:
                    instruction0x0A(registers, X, keypad);
                    break;
                case 0x29:
                    instruction0x29(I, X, registers);
                    break;
                case 0x33:
                    ram[*I] = registers[X] / 100;
                    ram[*I + 1] = (registers[X] % 100) / 10;
                    ram[*I + 2] = registers[X] % 10 ;
                    break;
                case 0x55:
                    for(int i = 0x0;i<=X;i++){
                        ram[*I + i] = registers[i];
                    }   
                    break;

                case 0x65:
                    for(int i = 0;i<=X;i++){
                        registers[i] = ram[*I + i];
                    }
                    break;
            }
            break;
    }
}

void ClearScreen(uint8_t *display, SDL_Renderer **renderer)
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

void DisplaySprite(uint8_t *registers, uint8_t *ram, uint8_t VX, uint8_t VY, 
                uint16_t N, uint16_t I, uint8_t *display){
    uint8_t x = registers[VX] % WIDTH;
    uint8_t y = registers[VY] % HEIGHT;

    registers[0xF] = 0;

    for(int row = 0;row < N;row++){
        uint8_t currByte = ram[I + row];

        for(int col = 0;col<8;col++){
            uint8_t spritePixel = currByte & (0x80u >> col);

            uint32_t pixelIndex = (y+row) * WIDTH + (x+col);

            if((x+col) >= WIDTH || (y+row) >= HEIGHT)
                continue;

            uint8_t currPixel = display[(y + row) * WIDTH + (x + col)];

            if(spritePixel){
                if (currPixel == 0xFF){
                    registers[0xF] = 1;
                }

                display[pixelIndex] ^= 0xFF;
            }
        }
    }
}

void DisplaySDL(uint8_t *display, SDL_Texture **texture, SDL_Renderer **renderer, int pitch)
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

void DecrementTimers(uint32_t *delay_timer, uint32_t *sound_timer){
    if(*delay_timer > 0){
        *delay_timer--;
    }

    if(*sound_timer > 0){
        *sound_timer--;
        Beep(750, 8);
    }

    SDL_Delay(16);
}

void Add(uint8_t *registers, uint16_t X, uint16_t Y){
    uint32_t res = registers[X] + registers[Y];
    registers[X] = res & 0xFF;

    registers[0xF] = (res > 255) ? 1 : 0;
}

void handleKeys(SDL_Event event, SDL_Scancode *keymap, uint8_t *keypad){
    for(int i = 0;i<CHIP8_KEY_COUNT;i++){
        if(event.key.keysym.scancode == keymap[i]){
            if(event.type == SDL_KEYDOWN){
                keypad[i] = 1;
            }else{
                keypad[i] = 0;
            }
            break;
        }
    }
}

bool isKeyPressed(uint8_t key, uint8_t *keypad){
    return keypad[key] == 1;
}

void instruction0x0A(uint8_t *registers, uint16_t X, uint8_t *keypad){
    uint8_t key_pressed = 0;
    uint8_t key_value = 0xFF;

    while (!key_pressed)
    {
        for (int i = 0; i < CHIP8_KEY_COUNT; i++)
        {
            if (isKeyPressed(i, keypad))
            {
                key_value = i;
                key_pressed = 1;
                break;
            }
        }
    }

    registers[X] = key_value;
}

void instruction0x29(uint16_t *I, uint16_t X, uint8_t *registers){    
    uint8_t char_index = registers[X] & 0X0F;
    *I = 0x050 + (char_index * 5);
}