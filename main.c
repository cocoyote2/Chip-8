#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <stdbool.h>

#define WIDTH 64
#define HEIGHT 32
#define SCALE 20
#define MEMORY_SIZE 4096
#define CLS 0x00E0
#define JMP 0x1000
#define SET_RG 0x6
#define ADD_VAL 0x7
#define SET_IND 0xA
#define DSPL 0xD

void Init(unsigned char* ram, unsigned char* registers, unsigned short *PC, unsigned short *I, unsigned short *stack, 
unsigned int *delay_timer, unsigned int *sound_timer);

void Fetch(unsigned short *opcode, unsigned char *ram, unsigned short *PC);

void ClearScreen(SDL_Renderer *renderer);

void DrawSprite(unsigned char *display, unsigned char *ram ,unsigned short I, uint8_t VX, uint8_t VY, uint8_t N, unsigned char *VF);

void DecrementTimer(unsigned int *timer);

void updateScreen(SDL_Renderer *renderer, unsigned char *display);

void handleEvents(SDL_Event event, bool *quit);

bool LoadRom(unsigned char *ram);

void Decode(unsigned char *display, unsigned char *ram, unsigned char *registers, 
unsigned short *I, unsigned short *PC, unsigned short opcode, SDL_Renderer *renderer);

int main(int argc, char *argv[])
{
    unsigned char ram[MEMORY_SIZE];
    unsigned char registers[16];
    //PC indicates where is the instruction
    unsigned short PC;
    unsigned short I;
    unsigned short stack;
    unsigned int delay_timer;
    unsigned int sound_timer;
    unsigned short opcode;
    unsigned char display[WIDTH*HEIGHT];
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool quit = false;
    bool drawFlag;

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

    Init(ram, registers, &PC, &I, &stack, &delay_timer, &sound_timer);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == 0){
        window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH*SCALE, HEIGHT*SCALE, SDL_WINDOW_RESIZABLE);  
        if(!window){
            printf("Unable to initialize window.");
            SDL_Quit();
            return 1;
        } 
        
        renderer = SDL_CreateRenderer(window, -1, 0);
        if(!renderer){
            printf("Unable to initialize renderer");
            SDL_Quit();
            return 1;
        }

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
        if(!texture){
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;   
        }
    }else{
        printf("Unable to initialize SDL.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }

    //Main loop
    while(!quit){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            handleEvents(event, &quit);

            LoadRom(ram);

            Fetch(&opcode, ram, &PC);

            Decode(display, ram, registers, &I, &PC, opcode, renderer);

            SDL_Delay(16);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Init(unsigned char* ram, unsigned char* registers, unsigned short *PC, unsigned short *I, unsigned short *stack, 
unsigned int *delay_timer, unsigned int *sound_timer){

    for(int i=0;i<MEMORY_SIZE;i++){
        ram[i] = 0;
    }

    for(int i = 0;i<16;i++){
        registers[i] = 0;
    }

    *PC = 0x200;
    *I = 0;
    *stack = 0;
    *delay_timer = 0;
    *sound_timer = 0;

    printf("Fin de l'initialisation.");
}

void Fetch(unsigned short *opcode, unsigned char *ram, unsigned short *PC){
    *opcode = ram[*PC] << 8 | ram[*PC+1];
    

    *PC+=2;
}

void ClearScreen(SDL_Renderer *renderer){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void DrawSprite(unsigned char *display, unsigned char *ram ,unsigned short I, uint8_t VX, uint8_t VY, uint8_t N, unsigned char *VF){
    uint8_t x = VX%WIDTH;
    uint8_t y = VY%HEIGHT;

    *VF = 0;

    for(int row=0;row<N;row++){
        unsigned char sprite = ram[I+row];

        for(int col =0;col<8;col++){
            if((sprite & (0x80 >> col)) != 0){
                int index = (x + col + ((y + row) * WIDTH)) % (WIDTH * HEIGHT);
                if(display[index] == 1){
                    *VF = 1;
                }
                display[index] ^= 1;
            }
        }
    }
}

void DecrementTimer(unsigned int *timer){
    while ((*timer) > 0)
    {
        (*timer)--;
        SDL_Delay(16);
    }
}

void updateScreen(SDL_Renderer *renderer, unsigned char *display){
    SDL_SetRenderDrawColor(renderer, 0,0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int y=0;y<HEIGHT;y++){
        for(int x=0;x<WIDTH;x++){
            if(display[y * WIDTH + x] == 1){
                SDL_Rect pixel = {
                    x*SCALE,
                    y*SCALE,
                    SCALE,
                    SCALE,

                };

                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void handleEvents(SDL_Event event, bool *quit){
    switch (event.type)
    {
    case SDL_QUIT:
        *quit = false;
        break;
    
    default:
        printf("Évènement non reconnu.");
        break;
    }
}

bool LoadRom(unsigned char *ram){
    FILE *rom = fopen( "./IBM_Logo.ch8","rb");

    if(rom == NULL){
        printf("Impossible d'ouvrir la rom.");
        return 1;
    }

    size_t rom_size = fread(ram + 0x200, sizeof(rom), MEMORY_SIZE - 0x200, rom);
    if(rom_size == 0){
        printf("Erreur dans la lecture de la rom.");
        fclose(rom);
        return 1;
    }

    fclose(rom);
}

void Decode(unsigned char *display, unsigned char *ram, unsigned char *registers, 
unsigned short *I, unsigned short *PC, unsigned short opcode, SDL_Renderer *renderer){
    // >> 8 to put the nibble at the end
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t N = (opcode & 0x000F);
    uint8_t NN = (opcode & 0x00FF);
    uint8_t NNN = (opcode & 0x0FFF);

    switch(opcode & 0xF0)
    {
        case 0x0000:
            if(opcode == 0x00E0){
                memset(display, 0, WIDTH*HEIGHT);
                ClearScreen(renderer);
            }
            break;      
        case 0x1000:
            *PC = NNN;
            break;
        case 0x6000:
            registers[X] = NN;
            break;
        case 0x7000:
            registers[X] += NN;
            break;
        case 0xA000:
            *I = NNN;
            break;
        case 0xD000:
            DrawSprite(display, ram, *I, X, Y, N, &registers[0xF]);
            updateScreen(renderer, display);
            break;
        default:
            printf("Incorrect instruction.");
    }
}