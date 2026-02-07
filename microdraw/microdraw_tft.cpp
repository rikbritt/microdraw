#include "microdraw.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#include <libraries/TFT_eSPI/TFT_eSPI.h>

const int FB_FPS_LIMIT = 4;


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

void blit_to_fb() {
    //SDL_Delay(1000 / FB_FPS_LIMIT);
}

struct MD_Image
{

};

struct MD_TFT_Image : public MD_Image
{
    ~MD_TFT_Image()
    {
        delete[] pixels;
        pixels = nullptr;
    }

    int32_t w = 0;
    int32_t h = 0;
    uint16_t key = 0;
    uint16_t* pixels = nullptr;
};

struct MicroDrawContext
{
    TFT_eSPI* tft = nullptr;
    bool exit_raised = false;
};

MicroDrawContext context;

bool md_init(int width, int height)
{
    return true;
}

void md_deinit()
{
    // TODO - leaks
    // clear context
}

MD_Image* md_load_image(const char* filename)
{
    // NOT SUPPORTED
    return (MD_Image*)nullptr;
}

MD_Image* md_load_image_with_key(const char* filename, uint8_t key_r, uint8_t key_g, uint8_t key_b)
{
    // NOT SUPPORTED
    return (MD_Image*)nullptr;
}

MD_Image* md_load_image_from_565_data(const char* data, int width, int height)
{
    MD_TFT_Image* new_image = (MD_TFT_Image*)md_create_image(width, height);
    memcpy(new_image->pixels, data, width * height * 2);
    return (MD_Image*)new_image;
}

MD_Image* md_load_image_from_565_data_with_key(const char* data, int width, int height, uint8_t key_r, uint8_t key_g, uint8_t key_b)
{
    MD_TFT_Image* new_image = (MD_TFT_Image*)md_create_image(width, height);
    memcpy(new_image->pixels, data, width * height * 2);
    new_image->key = context.tft->color565(key_r, key_g, key_b);
    return (MD_Image*)new_image;
}

MD_Image* md_create_image(int w, int h)
{
    MD_TFT_Image* new_image = new MD_TFT_Image();
    new_image->w = w;
    new_image->h = h;
    new_image->pixels = new uint16_t[w * h];
    return new_image;
}

void md_draw_pixel_to_image(MD_Image& image, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    //SDL_Surface* surface = (SDL_Surface*)&image;
    //Uint32* pixels = (Uint32*)surface->pixels;
    //const int pixelIdx = (y * surface->w) + x;
    //pixels[pixelIdx] = SDL_MapSurfaceRGB(surface, r, g, b);
}

void md_destroy_image(MD_Image& image)
{
    MD_TFT_Image* tft_image = (MD_TFT_Image*)&image;
    delete tft_image;
}

int md_get_image_width(const MD_Image& image)
{
    const MD_TFT_Image& tft_image = *(MD_TFT_Image*)&image;
    return tft_image.w;
}

int md_get_image_height(const MD_Image& image)
{
    const MD_TFT_Image& tft_image = *(MD_TFT_Image*)&image;
    return tft_image.h;
}

bool md_draw_image(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect)
{
    MD_TFT_Image& tft_image = *(MD_TFT_Image*)&image;
    //SDL_Surface* sdl_src = (SDL_Surface*)&image;
    //SDL_Rect* sdl_srcRect = (SDL_Rect*)srcRect;
    //SDL_Surface* sdl_dest = dest == nullptr ? context.canvas : (SDL_Surface*)dest;
    //SDL_Rect* sdl_destRect = (SDL_Rect*)destRect;
    //SDL_BlitSurface(sdl_src, sdl_srcRect, sdl_dest, sdl_destRect);
    const int32_t x = dest == nullptr ? 0 : destRect->x;
    const int32_t y = dest == nullptr ? 0 : destRect->y;
    const uint16_t* data = tft_image.pixels;
    // for 1 colour sprites?
    //context.tft->drawBitmap(x, y, bitmap, w, h,)
    context.tft->pushImage(x, y, tft_image.w, tft_image.h, data, tft_image.key);
    return true;
}

bool md_draw_image(MD_Image& image)
{
    return md_draw_image(image, nullptr, nullptr, nullptr);
}

bool md_draw_image_scaled(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect)
{
    //SDL_Surface* sdl_src = (SDL_Surface*)&image;
    //SDL_Rect* sdl_srcRect = (SDL_Rect*)srcRect;
    //SDL_Surface* sdl_dest = dest == nullptr ? context.canvas : (SDL_Surface*)dest;
    //SDL_Rect* sdl_destRect = (SDL_Rect*)destRect;
    //SDL_BlitSurfaceScaled(sdl_src, sdl_srcRect, sdl_dest, sdl_destRect, SDL_SCALEMODE_NEAREST);
    return true;
}

bool md_draw_image_scaled(MD_Image& image, MD_Rect& src, MD_Rect& dest)
{
    return md_draw_image_scaled(image, &src, nullptr, &dest);
}

void md_filled_rect(MD_Rect& rect, uint8_t r, uint8_t g, uint8_t b)
{
    context.tft->fillRect(rect.x, rect.y, rect.w, rect.h, context.tft->color565(r, g, b));
}

void md_set_image_clip(MD_Image& image, MD_Rect* rect)
{

    //SDL_Surface* sdl_src = (SDL_Surface*)&image;
    //SDL_Rect* sdl_clipRect = (SDL_Rect*)rect;
    //SDL_SetSurfaceClipRect(sdl_src, sdl_clipRect);
}

void md_set_clip(MD_Rect* rect)
{
    //md_set_image_clip(*(MD_Image*)context.canvas, rect);
}

void md_set_colour_mod(MD_Image& image, uint8_t key_r, uint8_t key_g, uint8_t key_b)
{
    //SDL_SetSurfaceColorMod((SDL_Surface*)&image, key_r, key_g, key_b);
}

//void GetPixelXBounds(SDL_Surface* surface, SDL_Rect rect, int& xLeftOut, int& xRightOut)
void md_get_pixel_x_bounds(MD_Image& image, const MD_Rect& rect, int& xLeftOut, int& xRightOut)
{
    //SDL_Surface* surface = (SDL_Surface*)&image;

    //// Ensure we don't read outside surface boundaries
    //int startX = std::max(0, rect.x);
    //int startY = std::max(0, rect.y);
    //int endX = std::min(surface->w, rect.x + rect.w);
    //int endY = std::min(surface->h, rect.y + rect.h);

    //if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

    //Uint32* pixels = (Uint32*)surface->pixels;
    //int pitch = surface->pitch / sizeof(Uint32);

    //// 1. Find Leftmost: Scan columns from left to right
    //xLeftOut = rect.w;
    //bool foundLeft = false;
    //for (int x = startX; x < endX && !foundLeft; ++x) {
    //    for (int y = startY; y < endY; ++y) {
    //        Uint32 pixel = pixels[y * pitch + x];
    //        // Check if pixel is not black (ignoring alpha channel)
    //        if ((pixel & 0x00FFFFFF) != 0) {
    //            xLeftOut = x - startX;
    //            foundLeft = true;
    //            break;
    //        }
    //    }
    //}

    //// 2. Find Rightmost: Scan columns from right to left
    //xRightOut = 0;
    //bool foundRight = false;
    //for (int x = endX - 1; x >= startX && !foundRight; --x) {
    //    for (int y = startY; y < endY; ++y) {
    //        Uint32 pixel = pixels[y * pitch + x];
    //        if ((pixel & 0x00FFFFFF) != 0) {
    //            xRightOut = x - startX;
    //            foundRight = true;
    //            break;
    //        }
    //    }
    //}

    //if (SDL_MUSTLOCK(surface))
    //{
    //    SDL_UnlockSurface(surface);
    //}
}

void md_render()
{
    //SDL_SetRenderDrawColor(context.ren, 255, 255, 255, 255);
    //SDL_RenderClear(context.ren);
    ////SDL_RenderCopy(ren, screen_tex, NULL, NULL);
    //SDL_UpdateTexture(context.screen_tex, NULL, context.canvas->pixels, context.canvas->pitch);
    //SDL_RenderTexture(context.ren, context.screen_tex, nullptr, nullptr);

    //// 4. Update Display
    //blit_to_fb(context.canvas);

    //SDL_RenderPresent(context.ren);


    //SDL_Event e;
    //while (SDL_PollEvent(&e))
    //{
    //    if (e.type == SDL_EVENT_QUIT)
    //    {
    //        context.exit_raised = true;
    //    }
    //}
}
//
//SDL_Surface* LoadBMPWithColorKey(const char* bmpName, SDL_PixelFormat format)
//{
//    SDL_Surface* temp = SDL_LoadBMP(bmpName);
//
//    // Force the bmp into the EXACT same format as our canvas (32-bit ARGB/XRGB)
//    SDL_Surface* bmp = SDL_ConvertSurface(temp, format);
//    SDL_DestroySurface(temp);
//
//    // Re-apply transparency on the NEW surface
//    SDL_SetSurfaceColorKey(bmp, true, SDL_MapSurfaceRGB(bmp, 0, 0, 0));
//    return bmp;
//}


bool md_exit_raised()
{
    return context.exit_raised;
}






