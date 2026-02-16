#include <microdraw.h>
#include "casio_v2.h"
#include "large_lcd_numbers.h"
#include "battery_0.h"
#include "battery_25.h"
#include "battery_50.h"
#include "battery_75.h"
#include "battery_100.h"
#include "battery_charging.h"

#ifndef WIN32
#include <LilyGoWatch.h>
#endif //WIN32
#include <ctime>

const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;
MD_Image* bg;
Font large_lcd_numbers;

MD_Image* img_battery_0;
MD_Image* img_battery_25;
MD_Image* img_battery_50;
MD_Image* img_battery_75;
MD_Image* img_battery_100;
MD_Image* img_battery_charging;

struct Time
{
    int hours;
    int minutes;
    int seconds;

    bool operator==(const Time& other) const
    {
        return hours == other.hours && minutes == other.minutes && seconds == other.seconds;
    }
    bool operator!=(const Time& other) const
    {
        return !(*this==other);
    }
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


float getNormalizedBattery() 
{
#ifdef WIN32
    static float level = 0.0f;
    level += 0.01f;
    return level;
#else
    TTGOClass* ttgo = TTGOClass::getWatch();
    // 1. Check if a battery is even connected
    if (!ttgo->power->isBatteryConnect()) {
        return 0.0f;
    }

    // 2. Get the voltage in milliVolts (e.g., 4200mV)
    uint16_t vbus_v = ttgo->power->getBattVoltage();
    
    // 3. Simple Linear Map (Approximation)
    // 3200mV is typically "dead", 4200mV is "full"
    float percentage = (vbus_v - 3200.0) / (4200.0 - 3200.0);
    
    // Constrain the value between 0.0 and 1.0
    if (percentage > 1.0) percentage = 1.0;
    if (percentage < 0.0) percentage = 0.0;
    
    return percentage;
#endif
}

enum class BatteryState
{
    Charging,
    ChargingFull,
    Discharging
};

BatteryState GetBatteryState()
{
#ifdef WIN32
    return BatteryState::Discharging;
#else
    TTGOClass* ttgo = TTGOClass::getWatch();
    if (ttgo->power->isVBUSPlug()) {
        if (ttgo->power->isChargeing()) {
            return BatteryState::Charging;
        } else {
            return BatteryState::ChargingFull;
        }
    } else {
        return BatteryState::Discharging;
    }
#endif
}

//void goToLightSleep()
//{
//    TTGOClass* ttgo = TTGOClass::getWatch();
//    // 1. Turn off the screen and backlight
//    ttgo->closeBL();
//    ttgo->displaySleep();
//
//    // 2. Tell the AXP202 to wake us up on a button press
//    // We use the PEK (Power Enable Key) interrupt
//    ttgo->power->setChgPwrUpTime(AXP202_PWROFF_4S); // Adjust timings
//    ttgo->power->enableIRQ(AXP202_PEK_SHORT_PRESS_IRQ, true);
//    ttgo->power->clearIRQ();
//
//    // 3. Enter Light Sleep
//    // This pauses the code right here
//    esp_light_sleep_start();
//
//    // --- The watch is now "off" ---
//    // After a button press, the code resumes here:
//
//    ttgo->displayWakeup();
//    ttgo->openBL();
//    ttgo->power->clearIRQ();
//}

Time watchLastTime;
void app_setup()
{

#ifndef WIN32
    TTGOClass* watch = TTGOClass::getWatch();
    watch->begin();
    watch->openBL();
    //Lower the brightness
    watch->bl->adjust(150);
    //watch->disableAudio();
    //watch->rtc->syncToSystem();
    setCpuFrequencyMhz(20);
#endif //WIN32

    if (!md_init(SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        return;
    }

    bg = md_load_image_from_565_data(casio_v2_data, casio_v2_width, casio_v2_height);
    large_lcd_numbers.InitFontFromImageData(large_lcd_numbers_data, large_lcd_numbers_width, large_lcd_numbers_height, 22, 55);
    large_lcd_numbers.m_NumbersOnly = true;

    img_battery_0 = md_load_image_from_565_data_with_key(battery_0_data, battery_0_width, battery_0_height, 0, 0, 0);
    md_set_colour_mod(*img_battery_0, 0, 0, 0);
    img_battery_25 = md_load_image_from_565_data_with_key(battery_25_data, battery_25_width, battery_25_height, 0, 0, 0);
    md_set_colour_mod(*img_battery_25, 0, 0, 0);
    img_battery_50 = md_load_image_from_565_data_with_key(battery_50_data, battery_50_width, battery_50_height, 0, 0, 0);
    md_set_colour_mod(*img_battery_50, 0, 0, 0);
    img_battery_75 = md_load_image_from_565_data_with_key(battery_75_data, battery_75_width, battery_75_height, 0, 0, 0);
    md_set_colour_mod(*img_battery_75, 0, 0, 0);
    img_battery_100 = md_load_image_from_565_data_with_key(battery_100_data, battery_100_width, battery_100_height, 0, 0, 0);
    md_set_colour_mod(*img_battery_100, 0, 0, 0);
    img_battery_charging = md_load_image_from_565_data_with_key(battery_charging_data, battery_charging_width, battery_charging_height, 0, 0, 0);
    md_set_colour_mod(*img_battery_charging, 0, 0, 0);

}

void draw_time_lcd(const Time& time)
{
    static char time_str[32];
    int x = 30;
    int y = 110;
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
}

void draw_battery_level()
{
    static int x = 170;
    static int y = 70;
    const BatteryState batteryState = GetBatteryState();
    MD_Image* battery_img = img_battery_0;
    switch(batteryState)
    {
        case BatteryState::Charging:
        case BatteryState::ChargingFull:
            battery_img = img_battery_charging;
            break;
        case BatteryState::Discharging:
        {
            const float batteryLevel = getNormalizedBattery();
            if(batteryLevel < 0.1f)
            {
                battery_img = img_battery_0;
            }
            else if(batteryLevel < 0.3f)
            {
                battery_img = img_battery_25;
            }
            else if(batteryLevel < 0.6f)
            {
                battery_img = img_battery_50;
            }
            else if(batteryLevel < 0.8f)
            {
                battery_img = img_battery_75;
            }
            else
            {
                battery_img = img_battery_100;
            }

            // static char time_str[32];
            // snprintf(time_str, sizeof(time_str), "%i", (int)(battery * 100.0f));
            // draw_num(large_lcd_numbers, 30, 100, time_str, 1);
        }
    }

    md_draw_image(*battery_img, x, y);
}

void checkSideButton()
{
#ifndef WIN32
    TTGOClass* ttgo = TTGOClass::getWatch();
    // Read the interrupt status from the PMU
    ttgo->power->readIRQ();

    if (ttgo->power->isPEKShortPressIRQ()) {
        // This code runs on a quick click
        //Serial.println("Short Press: Toggle Screen or Menu");
        // Example: Toggle backlight
        static bool blState = true;
        blState = !blState;
        if (blState) ttgo->openBL(); else ttgo->closeBL();
    }

    //if (ttgo->power->isPEKLongPressIRQ()) {
    //    // This code runs if held (usually ~1-2 seconds)
    //    Serial.println("Long Press: Power Menu or Deep Sleep");
    //}

    // CRITICAL: Clear the interrupts so the chip can detect the next press
    ttgo->power->clearIRQ();
#endif
}

void checkTouch()
{
#ifndef WIN32
    TTGOClass* ttgo = TTGOClass::getWatch();
    int16_t x, y;
    static int b = 0;
    static int framesTouched = 0;
    // Check if the screen is being touched
    if (ttgo->getTouch(x, y)) {
        // x and y now contain the coordinates (0-239)
        //Serial.printf("Touch at X: %d, Y: %d\n", x, y);

        // Example: Simple button area detection
        //if (x > 180 && y < 60) {
        //    Serial.println("Top-right corner touched!");
        //}

        ++framesTouched;
        if(framesTouched > 20)
        {
            b += 10;
            if(b > 255)
            {
                b = 0;
            }
            else if(b > 240)
            {
                b = 255;
            }
            ttgo->bl->adjust(b);
        }
    }
    else
    {
        framesTouched = 0;
    }
#endif
}

#ifdef WIN32
void delay(unsigned int ms)
{
    
}
#endif

void app_loop()
{
    checkSideButton();
    checkTouch();

    bool drawFace = false;

    Time currentTime = GetTime();
    if(currentTime != watchLastTime)
    {
        // Redraw the time
        drawFace = true;
        watchLastTime = currentTime;
    }
    else
    {
        delay(200);
    }

    if(drawFace)
    {
        if (bg)
        {
            md_draw_image(*bg);
        }


        // Draw for the next second
        draw_time_lcd(currentTime);
        draw_battery_level();
        //draw_num(canvas, large_lcd_numbers, 10, 10, "12345", 1);
        md_render();
    }
}
