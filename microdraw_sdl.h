#pragma once

//#ifdef MICRODRAW_SDL

#include <map>
#include <string>
#include <vector>

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_pixels.h>

struct SDL_Surface;
struct SDL_Renderer;
class Font;

struct MD_Image;
struct MD_Rect;

bool md_init_impl(int width, int height);
void md_deinit_impl();
MD_Image* md_load_image_impl(const char* filename);
MD_Image* md_load_image_with_key_impl(const char* filename, uint8_t key_r, uint8_t key_g, uint8_t key_b);
MD_Image* md_create_image_impl(int w, int h);
void md_draw_pixel_to_image_impl(MD_Image& image, int x, int y, uint8_t r, uint8_t g, uint8_t b);
void md_destroy_image_impl(MD_Image& image);
int md_get_image_width_impl(const MD_Image& image);
int md_get_image_height_impl(const MD_Image& image);
bool md_draw_image_impl(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect);
bool md_draw_image_scaled_impl(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect);
void md_filled_rect_impl(MD_Rect& rect, uint8_t r, uint8_t g, uint8_t b);
void md_set_image_clip_impl(MD_Image& image, MD_Rect* rect);
void md_set_clip_impl(MD_Rect* rect);
void md_set_colour_mod_impl(MD_Image& image, uint8_t key_r, uint8_t key_g, uint8_t key_b);
void md_get_pixel_x_bounds_impl(MD_Image& image, const MD_Rect& rect, int& xLeftOut, int& xRightOut);
void md_render_impl();
bool md_exit_raised_impl();

void blit_to_fb(SDL_Surface* surf);

typedef std::map<std::string, std::string> Values;
void LoadConfigToMap(const char* filename, Values& configMap);

SDL_Surface* LoadBMPWithColorKey(const char* bmpName, SDL_PixelFormat format);



/**
 * Converts HSL color values to SDL_Color (RGBA).
 * @param h Hue in degrees [0.0, 360.0]
 * @param s Saturation percentage [0.0, 1.0]
 * @param l Lightness percentage [0.0, 1.0]
 * @param a Alpha value [0, 255] (defaults to 255)
 */
SDL_Color HSLToSDLColor(float h, float s, float l, Uint8 a = 255);
int LerpInt(float t, int from, int to);


//#endif //MICRODRAW_SDL