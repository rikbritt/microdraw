#include <microdraw.h>
#include "casio_v2.h"
#include "large_lcd_numbers.h"

#ifndef WIN32
#include <LilyGoWatch.h>
#endif //WIN32
#include <ctime>

const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;
MD_Image* bg;
Font large_lcd_numbers;

struct Time
{
    int hours;
    int minutes;
    int seconds;
};

Time GetTime()
{
    Time out;
#ifdef WIN32
    time_t raw; time(&raw);
    struct tm* t = localtime(&raw);
    out.hours = t->tm_hour;
    out.minutes = t->tm_min;
    out.seconds = t->tm_sec;
#else
    RTC_Date curr_datetime = TTGOClass::getWatch()->rtc->getDateTime();
    out.hours = curr_datetime.hour;
    out.minutes = curr_datetime.minute;
    out.seconds = curr_datetime.second;
#endif //WIN32
    return out;
}


void app_setup()
{
#ifndef WIN32
    TTGOClass* watch = TTGOClass::getWatch();
    watch->begin();
    watch->openBL();
    //Lower the brightness
    watch->bl->adjust(150);
    //setCpuFrequencyMhz(20);
#endif //WIN32

    if (!md_init(SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        return;
    }

    bg = md_load_image_from_565_data(casio_v2_data, casio_v2_width, casio_v2_height);
    large_lcd_numbers.InitFontFromImageData(large_lcd_numbers_data, large_lcd_numbers_width, large_lcd_numbers_height, 22, 55);
    large_lcd_numbers.m_NumbersOnly = true;


}

void app_loop()
{
    if (bg)
    {
        md_draw_image(*bg);
    }


    static char time_str[32];
    const Time time = GetTime();
    int x = 30;
    int y = 100;
    md_set_colour_mod(*large_lcd_numbers.m_Surface, 109, 111, 98);
    snprintf(time_str, sizeof(time_str), "%02d", time.hours);
    draw_num(large_lcd_numbers, x, y, time_str, 1);
    snprintf(time_str, sizeof(time_str), "%02d", time.minutes);
    draw_num(large_lcd_numbers, x + 60, y, time_str, 1);
    snprintf(time_str, sizeof(time_str), "%02d", time.seconds);
    draw_num(large_lcd_numbers, x + 120, y, time_str, 1);
    md_set_colour_mod(*large_lcd_numbers.m_Surface, 255, 255, 255);

    x += 2;
    y += 2;
    md_set_colour_mod(*large_lcd_numbers.m_Surface, 0, 0, 0);
    snprintf(time_str, sizeof(time_str), "%02d", time.hours);
    draw_num(large_lcd_numbers, x, y, time_str, 1);
    snprintf(time_str, sizeof(time_str), "%02d", time.minutes);
    draw_num(large_lcd_numbers, x + 60, y, time_str, 1);
    snprintf(time_str, sizeof(time_str), "%02d", time.seconds);
    draw_num(large_lcd_numbers, x + 120, y, time_str, 1);
    md_set_colour_mod(*large_lcd_numbers.m_Surface, 255, 255, 255);

    //draw_num(canvas, large_lcd_numbers, 10, 10, "12345", 1);
    md_render();
}
