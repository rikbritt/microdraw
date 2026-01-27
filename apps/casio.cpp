#include "microdraw.h"

#define MICRODRAW_SDL
#include "casio/casio.h"
#include "casio/large_lcd_numbers.h"

#include <ctime>

const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;

int main(int argc, char* argv[])
{
    if (!md_init(SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        return -1;
    }

    MD_Image* bg = md_load_image_from_565_data(casio_data, casio_width, casio_height);
    Font large_lcd_numbers;
    large_lcd_numbers.InitFontFromImageData(large_lcd_numbers_data, large_lcd_numbers_width, large_lcd_numbers_height, 22, 55);
    large_lcd_numbers.m_NumbersOnly = true;


    bool run = true;
    int frame = 0;


    while (run)
    {
        run = md_exit_raised() == false;

        if (bg)
        {
            md_draw_image(*bg);
        }


        static char time_str[32];
        time_t raw; time(&raw);
        struct tm* t = localtime(&raw);
        int x = 30;
        int y = 100;
        md_set_colour_mod(*large_lcd_numbers.m_Surface, 109, 111, 98);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_hour);
        draw_num(large_lcd_numbers, x, y, time_str, 1);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_min);
        draw_num(large_lcd_numbers, x + 60, y, time_str, 1);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_sec);
        draw_num(large_lcd_numbers, x + 120, y, time_str, 1);
        md_set_colour_mod(*large_lcd_numbers.m_Surface, 255, 255, 255);

        x += 2;
        y += 2;
        md_set_colour_mod(*large_lcd_numbers.m_Surface, 0, 0, 0);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_hour);
        draw_num(large_lcd_numbers, x, y, time_str, 1);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_min);
        draw_num(large_lcd_numbers, x + 60, y, time_str, 1);
        snprintf(time_str, sizeof(time_str), "%02d", t->tm_sec);
        draw_num(large_lcd_numbers, x + 120, y, time_str, 1);
        md_set_colour_mod(*large_lcd_numbers.m_Surface, 255, 255, 255);

        //draw_num(canvas, large_lcd_numbers, 10, 10, "12345", 1);
        md_render();
    }

    md_deinit();
    return 0;
}