#include "microdraw.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef WIN32
#include <LilyGoWatch.h>
#include <libraries/TFT_eSPI/TFT_eSPI.h>
#endif //WIN32

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
    bool hasKey = false;
    bool hasMod = false;
    uint8_t mod_r = 0;
    uint8_t mod_g = 0;
    uint8_t mod_b = 0;
    uint16_t* pixels = nullptr;
};

uint16_t RGB565(uint8_t key_r, uint8_t key_g, uint8_t key_b)
{
    return ((key_r & 0xF8) << 8) | ((key_g & 0xFC) << 3) | (key_b >> 3);
}

inline void unpackRGB565(uint16_t color, uint8_t& r, uint8_t& g, uint8_t& b) {
    // 1. Extract the raw bits
    // Red:   Top 5 bits (Mask 0xF800, Shift Right 11)
    // Green: Middle 6 bits (Mask 0x07E0, Shift Right 5)
    // Blue:  Bottom 5 bits (Mask 0x001F)

    uint8_t r5 = (color >> 11) & 0x1F; // Range 0-31
    uint8_t g6 = (color >> 5) & 0x3F; // Range 0-63
    uint8_t b5 = color & 0x1F; // Range 0-31

    // 2. Expand to 8-bit (0-255)
    // We use bit shifting to scale efficiently without slow floating point math.
    // Logic: Copy the high bits into the low bits to fill the range.
    // Example: 11111 (31) -> 11111000 | 00000111 = 11111111 (255)

    r = (r5 << 3) | (r5 >> 2);
    g = (g6 << 2) | (g6 >> 4);
    b = (b5 << 3) | (b5 >> 2);
}

struct MicroDrawContext
{
    TFT_eSPI* tft = nullptr;
    TFT_eSprite* buffer = nullptr;
    bool exit_raised = false;
};

MicroDrawContext context;

bool md_init(int width, int height)
{
    context.tft = TTGOClass::getWatch()->tft;
    context.buffer = new TFT_eSprite(context.tft);
    context.buffer->createSprite(width, height, 1);
    context.buffer->setSwapBytes(true);
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
    MD_TFT_Image* img = (MD_TFT_Image*)md_create_image(width, height);
    memcpy(img->pixels, data, width * height * 2);
    img->key = context.tft->color565(key_r, key_g, key_b);
    img->hasKey = true;
    return (MD_Image*)img;
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
    const int imgW = md_get_image_width(image);
    const int imgH = md_get_image_height(image);

    if (!tft_image.hasKey)
    {
        if (srcRect == nullptr && destRect == nullptr)
        {
            context.buffer->pushImage(0, 0, imgW, imgH, (const uint16_t*)tft_image.pixels);
            return true;
        }
    }

    const int32_t x = destRect == nullptr ? 0 : destRect->x;
    const int32_t y = destRect == nullptr ? 0 : destRect->y;
    const int32_t srcX = srcRect == nullptr ? 0 : srcRect->x;
    const int32_t srcY = srcRect == nullptr ? 0 : srcRect->y;
    const int32_t srcW = srcRect == nullptr ? imgW : srcRect->w;
    const int32_t srcH = srcRect == nullptr ? imgH : srcRect->h;
    const uint16_t* data = tft_image.pixels;
    // for 1 colour sprites?
    //context.tft->drawBitmap(x, y, bitmap, w, h,)
    //context.tft->pushImage(x, y, tft_image.w, tft_image.h, data, tft_image.key);

    if (tft_image.hasMod)
    {
        for (int xIdx = 0; xIdx < srcW; ++xIdx)
        {
            for (int yIdx = 0; yIdx < srcH; ++yIdx)
            {
                int idx = srcX + xIdx + ((srcY + yIdx) * imgW);
                if (data[idx] != tft_image.key)
                {
                    uint8_t col_r, col_g, col_b;
                    unpackRGB565(data[idx], col_r, col_g, col_b);
                    const uint16_t col = RGB565((col_r * tft_image.mod_r) / 255, (col_g * tft_image.mod_g) / 255, (col_b * tft_image.mod_b) / 255);
                    context.buffer->drawPixel(x + xIdx, y + yIdx, col);
                }
            }
        }
    }
    else
    {
        for (int xIdx = 0; xIdx < srcW; ++xIdx)
        {
            for (int yIdx = 0; yIdx < srcH; ++yIdx)
            {
                int idx = srcX + xIdx + ((srcY + yIdx) * imgW);
                if (data[idx] != tft_image.key)
                {
                    context.buffer->drawPixel(x + xIdx, y + yIdx, data[idx]);
                }
            }
        }
    }

    return true;


    //MD_TFT_Image& tft_image = *(MD_TFT_Image*)&image;
    ////SDL_Surface* sdl_src = (SDL_Surface*)&image;
    ////SDL_Rect* sdl_srcRect = (SDL_Rect*)srcRect;
    ////SDL_Surface* sdl_dest = dest == nullptr ? context.canvas : (SDL_Surface*)dest;
    ////SDL_Rect* sdl_destRect = (SDL_Rect*)destRect;
    ////SDL_BlitSurface(sdl_src, sdl_srcRect, sdl_dest, sdl_destRect);
    //const int32_t x = dest == nullptr ? 0 : destRect->x;
    //const int32_t y = dest == nullptr ? 0 : destRect->y;
    //const uint16_t* data = tft_image.pixels;
    //// for 1 colour sprites?
    ////context.tft->drawBitmap(x, y, bitmap, w, h,)
    //context.tft->pushImage(x, y, tft_image.w, tft_image.h, data, tft_image.key);
    //return true;
}

bool md_draw_image(MD_Image& image)
{
    return md_draw_image(image, nullptr, nullptr, nullptr);
}

bool md_draw_image(MD_Image& image, int x, int y)
{
    MD_Rect dest;
    dest.x = x;
    dest.y = y;
    dest.w = md_get_image_width(image);
    dest.h = md_get_image_height(image);
    return md_draw_image(image, nullptr, nullptr, &dest);
}

bool md_draw_image_scaled(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect)
{
    //dumb and slow
    MD_TFT_Image& tft_image = *(MD_TFT_Image*)&image;
    MD_Rect src;
    if(srcRect)
    {
        src = *srcRect;
    }
    else
    {
        src.x = 0;
        src.y = 0;
        src.w = md_get_image_width(image);
        src.h = md_get_image_height(image);
    }

    MD_Rect dst;
    if(destRect)
    {
        dst = *destRect;
    }
    else
    {
        dst.x = 0;
        dst.y = 0;
    }
    for(int y=0; y<src.h; ++y)
    {
        for(int x=0; x<src.w; ++x)
        {
            const int srcX = src.x + x;
            const int srcY = src.y + y;
            const uint16_t col = tft_image.pixels[(srcY*tft_image.h) + srcX];
            if(col == tft_image.key)
            {
                continue;
            }
            context.tft->drawPixel(dst.x + x, dst.y + y, col);
        }
    }
    return true;
}

bool md_draw_image_scaled(MD_Image& image, MD_Rect& src, MD_Rect& dest)
{
    //temp ignore scaling
    return md_draw_image(image, &src, nullptr, &dest);
    //return md_draw_image_scaled(image, &src, nullptr, &dest);
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
    MD_TFT_Image& tft_image = *(MD_TFT_Image*)&image;
    if (key_r == 255 && key_g == 255 && key_b == 255)
    {
        tft_image.hasMod = false;
    }
    else
    {
        tft_image.hasMod = true;
        tft_image.mod_r = key_r;
        tft_image.mod_g = key_g;
        tft_image.mod_b = key_b;
    }
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
    context.buffer->pushSprite(0, 0);
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






