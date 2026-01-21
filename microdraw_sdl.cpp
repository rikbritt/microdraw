#include "microdraw_sdl.h"

#include "microdraw.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#endif

#include <SDL3/SDL.h>

const int FB_FPS_LIMIT = 4;

struct MicroDrawContext
{
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;
    SDL_Surface* canvas = nullptr;
    SDL_Texture* screen_tex = nullptr;
    bool exit_raised = false;
};

MicroDrawContext sdlContext;

bool md_init_impl(int width, int height)
{
    SDL_Init(SDL_INIT_VIDEO);
    sdlContext.win = SDL_CreateWindow("Pi Display", width, height, 0);
    sdlContext.ren = SDL_CreateRenderer(sdlContext.win, nullptr);


    // 1. Create the Surface for drawing (CPU side)
    sdlContext.canvas = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_XRGB8888);

    // 2. Create ONE Texture (GPU side) - Do this BEFORE the loop
    sdlContext.screen_tex = SDL_CreateTexture(sdlContext.ren,
        SDL_PIXELFORMAT_XRGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);
    return true;
}

void md_deinit_impl()
{
    // TODO - leaks
    // clear context
    SDL_Quit();
}


typedef struct {
    int fb_fd;
    unsigned short* fbp;
    int xres;
} FBInfo;


FBInfo fb_info = { -1, NULL, 0 };

bool init_fb() {
#ifdef __linux__
    fb_info.fb_fd = open("/dev/fb1", O_RDWR);
    if (fb_info.fb_fd == -1) return false;
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb_info.fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) return false;
    fb_info.xres = vinfo.xres;
    fb_info.fbp = (unsigned short*)mmap(0, vinfo.xres * vinfo.yres * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb_info.fb_fd, 0);
    return true;
#endif
    return true;
}

void blit_to_fb(SDL_Surface* surf) {
#ifdef __linux__
    if (!fb_info.fbp) return;
    Uint32* p = (Uint32*)surf->pixels;
    for (int y = 0; y < surf->h; y++) {
        for (int x = 0; x < surf->w; x++) {
            Uint32 c = p[y * surf->w + x];
            fb_info.fbp[y * fb_info.xres + x] = ((c >> 19) << 11) | (((c >> 10) & 0x3F) << 5) | (c >> 3 & 0x1F);
        }
    }
#else
    SDL_Delay(1000 / FB_FPS_LIMIT);
#endif
}



MD_Image* md_load_image_impl(const char* filename)
{
    SDL_Surface* image = SDL_LoadBMP(filename);
    return (MD_Image*)image;
}

MD_Image* md_load_image_with_key_impl(const char* filename, uint8_t key_r, uint8_t key_g, uint8_t key_b)
{
    SDL_Surface* temp_font = SDL_LoadBMP(filename);
    SDL_PixelFormat format = sdlContext.canvas->format;

    // Force the font into the EXACT same format as our canvas (32-bit ARGB/XRGB)
    SDL_Surface* surface = SDL_ConvertSurface(temp_font, format);
    SDL_DestroySurface(temp_font);

    // Re-apply transparency on the NEW surface
    SDL_SetSurfaceColorKey(surface, true, SDL_MapSurfaceRGB(surface, 0, 0, 0));
    return (MD_Image*)surface;
}

MD_Image* md_create_image_impl(int w, int h)
{
    SDL_Surface* new_surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    return (MD_Image*)new_surface;
}

void md_draw_pixel_to_image_impl(MD_Image& image, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    SDL_Surface* surface = (SDL_Surface*)&image;
    Uint32* pixels = (Uint32*)surface->pixels;
    const int pixelIdx = (y * surface->w) + x;
    pixels[pixelIdx] = SDL_MapSurfaceRGB(surface, r, g, b);
}

void md_destroy_image_impl(MD_Image& image)
{
    SDL_Surface* sdl_surface = (SDL_Surface*)&image;
    SDL_DestroySurface(sdl_surface);
}

int md_get_image_width_impl(const MD_Image& image)
{
    const SDL_Surface* sdl_src = (const SDL_Surface*)&image;
    return sdl_src->w;
}

int md_get_image_height_impl(const MD_Image& image)
{
    const SDL_Surface* sdl_src = (const SDL_Surface*)&image;
    return sdl_src->h;
}

bool md_draw_image_impl(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect)
{
    SDL_Surface* sdl_src = (SDL_Surface*)&image;
    SDL_Rect* sdl_srcRect = (SDL_Rect*)srcRect;
    SDL_Surface* sdl_dest = dest == nullptr ? sdlContext.canvas : (SDL_Surface*)dest;
    SDL_Rect* sdl_destRect = (SDL_Rect*)destRect;
    SDL_BlitSurface(sdl_src, sdl_srcRect, sdl_dest, sdl_destRect);
    return true;
}

bool md_draw_image_scaled_impl(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect)
{
    SDL_Surface* sdl_src = (SDL_Surface*)&image;
    SDL_Rect* sdl_srcRect = (SDL_Rect*)srcRect;
    SDL_Surface* sdl_dest = dest == nullptr ? sdlContext.canvas : (SDL_Surface*)dest;
    SDL_Rect* sdl_destRect = (SDL_Rect*)destRect;
    SDL_BlitSurfaceScaled(sdl_src, sdl_srcRect, sdl_dest, sdl_destRect, SDL_SCALEMODE_NEAREST);
    return true;
}

void md_filled_rect_impl(MD_Rect& rect, uint8_t r, uint8_t g, uint8_t b)
{
    SDL_Rect* sdl_rect = (SDL_Rect*)&rect;
    SDL_FillSurfaceRect(sdlContext.canvas, sdl_rect, SDL_MapSurfaceRGB(sdlContext.canvas, r, g, b));
}

void md_set_image_clip_impl(MD_Image& image, MD_Rect* rect)
{
    SDL_Surface* sdl_src = (SDL_Surface*)&image;
    SDL_Rect* sdl_clipRect = (SDL_Rect*)rect;
    SDL_SetSurfaceClipRect(sdl_src, sdl_clipRect);
}

void md_set_clip_impl(MD_Rect* rect)
{
    md_set_image_clip_impl(*(MD_Image*)sdlContext.canvas, rect);
}

void md_set_colour_mod_impl(MD_Image& image, uint8_t key_r, uint8_t key_g, uint8_t key_b)
{
    SDL_SetSurfaceColorMod((SDL_Surface*)&image, key_r, key_g, key_b);
}

//void GetPixelXBounds(SDL_Surface* surface, SDL_Rect rect, int& xLeftOut, int& xRightOut)
void md_get_pixel_x_bounds_impl(MD_Image& image, const MD_Rect& rect, int& xLeftOut, int& xRightOut)
{
    SDL_Surface* surface = (SDL_Surface*)&image;

    // Ensure we don't read outside surface boundaries
    int startX = std::max(0, rect.x);
    int startY = std::max(0, rect.y);
    int endX = std::min(surface->w, rect.x + rect.w);
    int endY = std::min(surface->h, rect.y + rect.h);

    if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

    Uint32* pixels = (Uint32*)surface->pixels;
    int pitch = surface->pitch / sizeof(Uint32);

    // 1. Find Leftmost: Scan columns from left to right
    xLeftOut = rect.w;
    bool foundLeft = false;
    for (int x = startX; x < endX && !foundLeft; ++x) {
        for (int y = startY; y < endY; ++y) {
            Uint32 pixel = pixels[y * pitch + x];
            // Check if pixel is not black (ignoring alpha channel)
            if ((pixel & 0x00FFFFFF) != 0) {
                xLeftOut = x - startX;
                foundLeft = true;
                break;
            }
        }
    }

    // 2. Find Rightmost: Scan columns from right to left
    xRightOut = 0;
    bool foundRight = false;
    for (int x = endX - 1; x >= startX && !foundRight; --x) {
        for (int y = startY; y < endY; ++y) {
            Uint32 pixel = pixels[y * pitch + x];
            if ((pixel & 0x00FFFFFF) != 0) {
                xRightOut = x - startX;
                foundRight = true;
                break;
            }
        }
    }

    if (SDL_MUSTLOCK(surface))
    {
        SDL_UnlockSurface(surface);
    }
}

void md_render_impl()
{
    SDL_SetRenderDrawColor(sdlContext.ren, 255, 255, 255, 255);
    SDL_RenderClear(sdlContext.ren);
    //SDL_RenderCopy(ren, screen_tex, NULL, NULL);
    SDL_UpdateTexture(sdlContext.screen_tex, NULL, sdlContext.canvas->pixels, sdlContext.canvas->pitch);
    SDL_RenderTexture(sdlContext.ren, sdlContext.screen_tex, nullptr, nullptr);

    // 4. Update Display
    blit_to_fb(sdlContext.canvas);

    //static SDL_Rect blockout;
    //blockout.x = 13;
    //blockout.y = 250;
    //blockout.w = 134;
    //blockout.h = 121;
    //SDL_SetRenderDrawColor(ren, 100, 0, 0, 255);
    //SDL_RenderFillRect(ren, &blockout);

    SDL_RenderPresent(sdlContext.ren);


    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            sdlContext.exit_raised = true;
        }
    }
}

bool md_exit_raised_impl()
{
    return sdlContext.exit_raised;
}








