#include <SDL3/SDL.h>

#include <fstream>
#include <iostream>

#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#endif

#include "microdraw.h"

const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;

int main(int argc, char* argv[])
{
    srand((unsigned int)time(NULL));


    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow("Pi Display", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, nullptr);

    // 1. Create the Surface for drawing (CPU side)
    SDL_Surface* canvas = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_XRGB8888);

    // 2. Create ONE Texture (GPU side) - Do this BEFORE the loop
    SDL_Texture* screen_tex = SDL_CreateTexture(ren,
        SDL_PIXELFORMAT_XRGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_Surface* bg = SDL_LoadPNG("casio.png");
    Font large_lcd_numbers;
    large_lcd_numbers.InitFont("large_lcd_numbers.png", 22, 55, canvas->format);
    large_lcd_numbers.m_NumbersOnly = true;

    if (!init_fb())
    {
        return -1;
    }

    bool run = true;
    int frame = 0;


    while (run)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                run = false;
            }
        }

        if (bg)
        {
            SDL_BlitSurface(bg, NULL, canvas, NULL);
        }


        static char time_str[32];
        time_t raw; time(&raw);
        struct tm* t = localtime(&raw);
        int x = 30;
        int y = 100;
        SDL_SetSurfaceColorMod(large_lcd_numbers.m_Surface, 109, 111, 98);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_hour);
        draw_num(canvas, large_lcd_numbers, x, y, time_str, 1);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_min);
        draw_num(canvas, large_lcd_numbers, x + 60, y, time_str, 1);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_sec);
        draw_num(canvas, large_lcd_numbers, x + 120, y, time_str, 1);
        SDL_SetSurfaceColorMod(large_lcd_numbers.m_Surface, 255, 255, 255);

        x += 2;
        y += 2;
        SDL_SetSurfaceColorMod(large_lcd_numbers.m_Surface, 0, 0, 0);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_hour);
        draw_num(canvas, large_lcd_numbers, x, y, time_str, 1);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_min);
        draw_num(canvas, large_lcd_numbers, x + 60, y, time_str, 1);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_sec);
        draw_num(canvas, large_lcd_numbers, x + 120, y, time_str, 1);
        SDL_SetSurfaceColorMod(large_lcd_numbers.m_Surface, 255, 255, 255);

        //draw_num(canvas, large_lcd_numbers, 10, 10, "12345", 1);

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);
        //SDL_RenderCopy(ren, screen_tex, NULL, NULL);
        SDL_UpdateTexture(screen_tex, NULL, canvas->pixels, canvas->pitch);
        SDL_RenderTexture(ren, screen_tex, nullptr, nullptr);

        // 4. Update Display
        blit_to_fb(canvas);

        //static SDL_Rect blockout;
        //blockout.x = 13;
        //blockout.y = 250;
        //blockout.w = 134;
        //blockout.h = 121;
        //SDL_SetRenderDrawColor(ren, 100, 0, 0, 255);
        //SDL_RenderFillRect(ren, &blockout);

        SDL_RenderPresent(ren);
    }

    SDL_Quit();
    return 0;
}