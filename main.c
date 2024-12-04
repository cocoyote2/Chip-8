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

void Init(unsigned char* ram, unsigned char *display, unsigned char* registers, unsigned short *PC, unsigned short *I, unsigned short *stack, 
unsigned int *delay_timer, unsigned int *sound_timer, unsigned char *sp);

void Fetch(unsigned short *opcode, unsigned char *ram, unsigned short *PC);

void ClearScreen(SDL_Renderer *renderer);

void DrawSprite(unsigned char *display, unsigned char *ram ,unsigned short I, uint8_t VX, uint8_t VY, uint8_t N, unsigned char *VF);

void DecrementTimer(unsigned int *timer);

void updateScreen(SDL_Renderer *renderer, unsigned char *display);

void handleEvents(SDL_Event event, bool *quit);

bool LoadRom(unsigned char *ram);

void Decode(unsigned char *display, unsigned char *ram, unsigned char *registers, 
unsigned short *I, unsigned short *PC, unsigned short opcode, SDL_Renderer *renderer);

void UpdateTimers(unsigned int *delay_timer, unsigned int *sound_timer);

bool InitSDL(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture);

int main(int argc, char *argv[])
{
    unsigned char ram[MEMORY_SIZE];
    unsigned char registers[16];
    //PC indicates where is the instruction
    unsigned short PC;
    unsigned short I;
    unsigned short stack[16];
    unsigned char sp;
    unsigned int delay_timer;
    unsigned int sound_timer;
    unsigned short opcode;
    unsigned char display[WIDTH*HEIGHT] = {0};
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

    Init(ram, registers, display,&PC, &I, stack, &delay_timer, &sound_timer, &sp);

    memcpy(ram + 0x050, font, sizeof(font));

    if (LoadRom(ram))
    {
        printf("Rom loaded !\n");
        fflush(stdout); // Force l'affichage immédiat
        printf("Contenu de la RAM après le chargement de la ROM :\n");
        fflush(stdout); // Force l'affichage immédiat
        for (int i = 0x200; i < 0x210; i++)
        {
            printf("0x%04x: 0x%02X\n", i, ram[i]);
            fflush(stdout); // Force l'affichage immédiat
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == 0)
    {
        window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_RESIZABLE);
        if (!window)
        {
            printf("Unable to initialize window.");
            fflush(stdout);
            SDL_Quit();
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, 0);
        if (!renderer)
        {
            printf("Unable to initialize renderer");
            fflush(stdout);
            SDL_Quit();
            return false;
        }

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
        if (!texture)
        {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
        }
    }
    else
    {
        printf("Unable to initialize SDL.\n");
        fflush(stdout);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return false;
    }

    const int TIMER_INTERVAL = 1000 / 60; // 1/60e de seconde en millisecondes
    Uint32 last_timer_update = SDL_GetTicks();

        // Main loop
    while (!quit)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            handleEvents(event, &quit);

            Uint32 current_time = SDL_GetTicks();
            if (current_time - last_timer_update >= TIMER_INTERVAL)
            {
                UpdateTimers(&delay_timer, &sound_timer);
                last_timer_update = current_time;
            }

            Fetch(&opcode, ram, &PC);

            Decode(display, ram, registers, &I, &PC, opcode, renderer);

            SDL_Delay(16);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

bool InitSDL(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture){
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == 0)
    {
        window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_RESIZABLE);
        if (!window)
        {
            printf("Unable to initialize window.");
            fflush(stdout);
            SDL_Quit();
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, 0);
        if (!renderer)
        {
            printf("Unable to initialize renderer");
            fflush(stdout);
            SDL_Quit();
            return false;
        }

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
        if (!texture)
        {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
        }
    }
    else
    {
        printf("Unable to initialize SDL.\n");
        fflush(stdout);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return false;
    }
}

void Init(unsigned char* ram, unsigned char *display, unsigned char* registers, unsigned short *PC, unsigned short *I, unsigned short *stack, 
unsigned int *delay_timer, unsigned int *sound_timer, unsigned char *sp){

    for(int i=0;i<MEMORY_SIZE;i++){
        ram[i] = 0;
    }

    for(int i = 0;i<16;i++){
        registers[i] = 0;
    }

    *PC = 0x200;
    *I = 0;
    *sp = 0;
    *delay_timer = 0;
    *sound_timer = 0;
    memset(display, 0, WIDTH * HEIGHT);

    printf("Fin de l'initialisation.");
    fflush(stdout);
}

void Fetch(unsigned short *opcode, unsigned char *ram, unsigned short *PC){
    *opcode = ram[*PC] << 8 | ram[*PC+1];
    

    *PC+=2;
}

void ClearScreen(SDL_Renderer *renderer){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void DrawSprite(unsigned char *display, unsigned char *ram, unsigned short I, uint8_t VX, uint8_t VY, uint8_t N, unsigned char *VF)
{
    uint8_t x = VX % WIDTH;
    uint8_t y = VY % HEIGHT;

    *VF = 0; // Réinitialiser VF avant de dessiner

    for (int row = 0; row < N; row++)
    {
        // Vérifier les limites de la mémoire
        if (I + row >= MEMORY_SIZE)
        {
            printf("Erreur : Lecture hors limite de la mémoire (I=%d, row=%d)\n", I, row);
            break;
        }

        unsigned char sprite = ram[I + row];

        for (int col = 0; col < 8; col++)
        {
            // Vérifier que les coordonnées sont dans les limites de l'écran
            if (x + col < WIDTH && y + row < HEIGHT)
            {
                int index = (x + col) + ((y + row) * WIDTH);

                if ((sprite & (0x80 >> col)) != 0)
                { // Vérifier le bit correspondant
                    if (display[index] == 1)
                    {
                        *VF = 1; // Collision détectée
                    }
                    display[index] ^= 1; // Inverser l'état du pixel
                }
            }
        }
    }
}

void UpdateTimers(unsigned int *delay_timer, unsigned int *sound_timer)
{
    if (*delay_timer > 0)
    {
        (*delay_timer)--;
    }
    if (*sound_timer > 0)
    {
        (*sound_timer)--;

        // Jouer un son si sound_timer atteint 0
        if (*sound_timer == 0)
        {
            printf("BEEP!\n"); // Remplacez par une bibliothèque audio pour jouer un vrai son
        }
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
        *quit = true;
        break;
    
    default:
        printf("Évènement non reconnu.");
        break;
    }
}

bool LoadRom(unsigned char *ram){
    FILE *rom = fopen("./test_opcode.ch8", "rb");

    if(rom == NULL){
        printf("Impossible d'ouvrir la rom.");
        return false;
    }

    size_t rom_size = fread(ram + 0x200, 1, MEMORY_SIZE - 0x200, rom);

    if(rom_size == 0){
        printf("Erreur dans la lecture de la rom.");
        fclose(rom);
        return false;
    }

    fclose(rom);
    return true;
}

void Decode(unsigned char *display, unsigned char *ram, unsigned char *registers, 
unsigned short *I, unsigned short *PC, unsigned short opcode, SDL_Renderer *renderer){
    // >> 8 to put the nibble at the end
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t N = (opcode & 0x000F);
    uint8_t NN = (opcode & 0x00FF);
    uint8_t NNN = (opcode & 0x0FFF);

    switch(opcode & 0xF000)
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
            DrawSprite(display, ram, *I, registers[X], registers[Y], N, &registers[0xF]);
            updateScreen(renderer, display);
            break;
        default:
            printf("Incorrect instruction.");
    }
}