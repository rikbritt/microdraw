#include <SDL3/SDL.h>

#include "microdraw.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>

#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#endif


const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 480;

void UpdateClock(int x, int y, SDL_Surface* dest, Font& font)
{
    static char time_str[32];
    time_t raw; time(&raw);
    struct tm* t = localtime(&raw);
    snprintf(time_str, sizeof(time_str), "%02d", t->tm_hour);
    draw_num(dest, font, x, y, time_str, 2);
    snprintf(time_str, sizeof(time_str), "%02d", t->tm_min);
    draw_num(dest, font, x + 80, y, time_str, 2);
    snprintf(time_str, sizeof(time_str), "%02d", t->tm_sec);
    draw_num(dest, font, x + 160, y, time_str, 2);
}

struct TextWall
{
    void InitTextWall(int x, int y, int w, int h, Font& font)
    {
        m_Rect.x = x;
        m_Rect.y = y;
        m_Rect.w = w;
        m_Rect.h = h;
        m_Font = &font;

        m_CharW = w / font.m_GlyphSurfaceW;
        m_CharH = w / font.m_GlyphSurfaceH;

        m_Text = new char[m_CharW * m_CharH];
        memset(m_Text, 0, m_CharW * m_CharH);
    }

    ~TextWall()
    {
        delete[] m_Text;
        m_Text = nullptr;
    }

    void AddToTopOfWall(const char* text);
    void SetLine(int line, const char* text)
    {
        const int text_len = (int)strlen(text);
        const int copy_len = text_len > m_CharW ? m_CharW : text_len;
        memset(m_Text + (line * m_CharW), 0, m_CharW);
        memcpy(m_Text + (line * m_CharW), text, copy_len);
    }
    void SetWrappedLine(int line, const char* text);
    void Clear()
    {
        memset(m_Text, 0, m_CharW * m_CharH);
    }
    void DrawTextWall(SDL_Surface* dest, SDL_Color col)
    {
        static char line[128];
        SDL_SetSurfaceColorMod(m_Font->m_Surface, col.r, col.g, col.b);
        for (int y = 0; y < m_CharH; ++y)
        {
            memcpy(line, &m_Text[y * m_CharW], m_CharW);
            line[m_CharW] = '\0';
            draw_text(dest, *m_Font, m_Rect.x, m_Rect.y + (y * m_Font->m_GlyphSurfaceH), line, 1);
        }
        SDL_SetSurfaceColorMod(m_Font->m_Surface, 255, 255, 255);
    }

    SDL_Rect m_Rect;
    Font* m_Font = nullptr;
    char* m_Text = nullptr;
    int m_CharW = 0;
    int m_CharH = 0;
};

void TextWall::SetWrappedLine(int line, const char* text)
{
    const char* cursor = text;
    int charsLeft = (int)strlen(text);
    while (charsLeft > 0)
    {
        if (line >= m_CharH)
        {
            // Out of space
            break;
        }

        char* currLine = m_Text + (line * m_CharW);
        memset(currLine, 0, m_CharW);

        if (charsLeft <= m_CharW)
        {
            // Write all that's left
            memcpy(currLine, cursor, charsLeft);
            break;
        }

        // Find closest next break
        int nextBreak = 0;
        int skip = 0;
        for (int i = 0; i < m_CharW; ++i)
        {
            if (cursor[i] == ' ')
            {
                nextBreak = i;
                skip = 1; //skip space
            }
        }

        if (nextBreak == 0)
        {
            // no break, fake a position
            nextBreak = m_CharW;
        }

        // copy to break
        memcpy(currLine, cursor, nextBreak);
        charsLeft -= nextBreak;
        cursor += nextBreak + skip;
        line += 1;
        
    }
}

void TextWall::AddToTopOfWall(const char* text)
{
    // Shuffle existing lines down
    for (int y = m_CharH - 1; y > 0; --y)
    {
        memset(m_Text + (y * m_CharW), 0, m_CharW);
        memcpy(m_Text + (y * m_CharW), m_Text + ((y - 1) * m_CharW), m_CharW);
    }

    SetLine(0, text);
}

void drawSpinningCube(char* ptr, int width, int height, float A, float B, float C)
{
    // Buffers and settings
    static float* zBuffer = nullptr;
    if (zBuffer == nullptr)
    {
        zBuffer = new float[width * height];
    }
    memset(zBuffer, 0, sizeof(float)* width* height);
    std::memset(ptr, ' ', width * height); // Fill background with spaces

    const float cubeWidth = 15.0f;
    const float incrementSpeed = 0.6f;
    const int distanceFromCam = 100;
    const float K1 = 30.0f; // Field of view / scaling factor

    auto calculateX = [&](float i, float j, float k) {
        return j * sin(A) * sin(B) * cos(C) - k * cos(A) * sin(B) * cos(C) +
            j * cos(A) * sin(C) + k * sin(A) * sin(C) + i * cos(B) * cos(C);
        };

    auto calculateY = [&](float i, float j, float k) {
        return j * cos(A) * cos(C) + k * sin(A) * cos(C) -
            j * sin(A) * sin(B) * sin(C) + k * cos(A) * sin(B) * sin(C) -
            i * cos(B) * sin(C);
        };

    auto calculateZ = [&](float i, float j, float k) {
        return k * cos(A) * cos(B) - j * sin(A) * cos(B) + i * sin(B);
        };

    auto projectSurface = [&](float cubeX, float cubeY, float cubeZ, char ch) {
        float x = calculateX(cubeX, cubeY, cubeZ);
        float y = calculateY(cubeX, cubeY, cubeZ);
        float z = calculateZ(cubeX, cubeY, cubeZ) + distanceFromCam;

        float ooz = 1.0f / z; // "One over Z" for depth

        // Calculate 2D projection
        int xp = (int)(width / 2 + K1 * ooz * x * 1); // Factor of 2 to account for font aspect ratio
        int yp = (int)(height / 2 + K1 * ooz * y);

        if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
            if (ooz > zBuffer[yp * width + xp]) {
                zBuffer[yp * width + xp] = ooz;
                ptr[yp * width + xp] = ch;
            }
        }
        };

    // Iterate through cube faces
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += incrementSpeed) {
        for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += incrementSpeed) {
            projectSurface(cubeX, cubeY, cubeWidth, '@');  // Front
            projectSurface(cubeX, cubeY, -cubeWidth, '.'); // Back
            projectSurface(cubeWidth, cubeY, cubeX, 'X');  // Right
            projectSurface(-cubeWidth, cubeY, cubeX, '~'); // Left
            projectSurface(cubeX, cubeWidth, cubeY, '#');  // Top
            projectSurface(cubeX, -cubeWidth, cubeY, ';'); // Bottom
        }
    }
}

class FeedView
{
public:
    void UpdateFeed(TextWall& textWall, const Values& values);
};

void FeedView::UpdateFeed(TextWall& textWall, const Values& values)
{
    //Scroll lines
    //for (int y = 1; y < m_CharH; ++y)
    //{
    //    memcpy(m_FeedText + ((y - 1) * m_CharW), m_FeedText + (y * m_CharW), m_CharW);
    //}
    //memset(m_FeedText + ((m_CharH - 1) * m_CharW), 0, m_CharW);

    // Count num feed lines in text file
    int count = 0;
    for (auto& v : values)
    {
        if (v.first.find_first_of("feed") == 0)
        {
            ++count;
        }
    }

    const char* new_feed_line = " - - - - - ";
    if (count > 0)
    {
        int selected = rand() % count;
        for (auto& v : values)
        {
            if (v.first.find_first_of("feed") == 0)
            {
                if (selected == 0)
                {
                    new_feed_line = v.second.c_str();
                    break;
                }
                else
                {
                    --selected;
                }
            }
        }
    }

    textWall.AddToTopOfWall(new_feed_line);
}

class UselessFactData
{
public:
    bool UpdateUselessFact()
    {
        const std::string old_fact = m_UselessFact;

        ParseJSONFile("uselessfact.json", m_Doc);
        m_UselessFact = m_Doc.GetObject("text").GetAsString();

        if ((old_fact != m_UselessFact))
        {
            return true;
        }

        return false;
    }

    JSONVal m_Doc;
    std::string m_UselessFact;
};


class WeatherData
{
public:

    bool UpdateWeatherData()
    {
        const int old_CurrentWeatherCode = m_CurrentWeatherCode;
        const float old_CurrentTemp = m_CurrentTemp;
        const float old_TempMin = m_TempMin;
        const float old_TempMax = m_TempMax;

        ParseJSONFile("weather.json", m_WeatherDoc);
        m_CurrentWeatherCode = m_WeatherDoc.GetObject("current").GetObject("weather_code").GetAsInt();
        m_CurrentWeatherDesc = GetWeatherDescription(m_CurrentWeatherCode);
        m_CurrentTemp = m_WeatherDoc.GetObject("current").GetObject("temperature_2m").GetAsFloat();
        m_TempMin = m_WeatherDoc.GetObject("daily").GetObject("temperature_2m_min").GetArrayVal(0).GetAsFloat();
        m_TempMax = m_WeatherDoc.GetObject("daily").GetObject("temperature_2m_max").GetArrayVal(0).GetAsFloat();

        if ((old_CurrentWeatherCode != m_CurrentWeatherCode)
            || (old_CurrentTemp != m_CurrentTemp)
            || (old_TempMin != m_TempMin)
            || (old_TempMax != m_TempMax)
            )
        {
            return true;
            }

        return false;
    }

    /**
     * Converts a Met Office DataPoint weather code to a short descriptive string.
     * Reference: https://www.metoffice.gov.uk/services/data/datapoint/code-definitions
     */
    const char* GetWeatherDescription(int code) {
        switch (code) {
        case -1: return "Trace rain";
        case 0:  return "Clear night";
        case 1:  return "Sunny day";
        case 2:  return "Partly cloudy (night)";
        case 3:  return "Partly cloudy (day)";
        case 4:  return "Not used";
        case 5:  return "Mist";
        case 6:  return "Fog";
        case 7:  return "Cloudy";
        case 8:  return "Overcast";
        case 9:  return "Light rain shower (night)";
        case 10: return "Light rain shower (day)";
        case 11: return "Drizzle";
        case 12: return "Light rain";
        case 13: return "Heavy rain shower (night)";
        case 14: return "Heavy rain shower (day)";
        case 15: return "Heavy rain";
        case 16: return "Sleet shower (night)";
        case 17: return "Sleet shower (day)";
        case 18: return "Sleet";
        case 19: return "Hail shower (night)";
        case 20: return "Hail shower (day)";
        case 21: return "Hail";
        case 22: return "Light snow shower (night)";
        case 23: return "Light snow shower (day)";
        case 24: return "Light snow";
        case 25: return "Heavy snow shower (night)";
        case 26: return "Heavy snow shower (day)";
        case 27: return "Heavy snow";
        case 28: return "Thunder shower (night)";
        };

        return "UNKNOWN";
    }

    JSONVal m_WeatherDoc;
    int m_CurrentWeatherCode = 0;
    std::string m_CurrentWeatherDesc;
    float m_CurrentTemp = 0.0f;
    float m_TempMin = 0.0f;
    float m_TempMax = 0.0f;
};

class ControlIndicator
{
public:
    void InitControlIndicator(int stackSize, int x, int y, SDL_Surface& controlBar)
    {
        m_StackSize = stackSize;
        //m_State = new bool[stackSize];
        m_NumActive = 0;
        m_ControlBar = &controlBar;
        m_X = x;
        m_Y = y;
    }

    ~ControlIndicator()
    {
        //delete m_State;
    }

    void UpdateAndDraw(SDL_Surface* dest)
    {
        SDL_Rect destRect = { m_X, m_Y, m_ControlBar->w, m_ControlBar->h };

        if (rand() % 3 == 0)
        {
            int change = (rand() % 4) - (2 + m_Trend);
            m_NumActive += change;
            if (m_NumActive < 0)
            {
                m_NumActive = 0;
            }
            if (m_NumActive > m_StackSize)
            {
                m_NumActive = m_StackSize;
            }
        }

        if (rand() % 5 == 0)
        {
            m_Trend = (rand() % 3) - 1;
        }

        for (int i = 0; i < m_StackSize; ++i)
        {
            SDL_Color col = m_OnColour;
            if (i < m_NumActive)
            {
                const float t = (float)i / (float)m_NumActive;
                col.r = LerpInt(t, m_OffColour.r, m_OnColour.r);
                col.g = LerpInt(t, m_OffColour.g, m_OnColour.g);
                col.b = LerpInt(t, m_OffColour.b, m_OnColour.b);
            }

            SDL_SetSurfaceColorMod(m_ControlBar, col.r, col.g, col.b);
            
            SDL_BlitSurface(m_ControlBar, NULL, dest, &destRect);
            destRect.y += destRect.h + 2;
        }
    }

    SDL_Surface* m_ControlBar = nullptr; // NO OWNERSHIP
    int m_StackSize = 0;
    int m_X = 0;
    int m_Y = 0;
    int m_NumActive = 0;
    int m_Trend = 0;
    //bool* m_State = nullptr;
    SDL_Color m_OnColour = { 0, 255, 48 };
    SDL_Color m_OffColour = { 255, 0, 0 };
};

struct ReactorCell
{
    int x = 0;
    int y = 0;
    bool on = false;
};

class Gradient
{
public:
    ~Gradient();
    void Reset();
    void InitGradient(SDL_Renderer* renderer, SDL_Color startColor, SDL_Color endColor, const SDL_Rect& dstRect);
    void RenderGradient(SDL_Renderer* renderer, SDL_Surface* dest);
    SDL_Surface* m_Surface = nullptr;
    SDL_Rect m_Rect{ 0,0,0,0 };
};

Gradient::~Gradient()
{
    Reset();
}

void Gradient::Reset()
{
    SDL_DestroySurface(m_Surface);
    m_Surface = nullptr;
}

void Gradient::InitGradient(SDL_Renderer* renderer, SDL_Color startColor, SDL_Color endColor, const SDL_Rect& dstRect)
{
    if (m_Surface)
    {
        Reset();
    }

    m_Rect = dstRect;
    const int height = m_Rect.h;

    SDL_Surface* surface = SDL_CreateSurface(1, height, SDL_PIXELFORMAT_RGBA32);
    m_Surface = surface;

    // 2. Access the raw pixels
    Uint32* pixels = (Uint32*)surface->pixels;
    if (!pixels)
    {
        return;
    }
    for (int y = 0; y < height; ++y) {
        // Calculate the interpolation factor (0.0 to 1.0)
        float t = (float)y / (float)(height - 1);

        // 3. Interpolate R, G, B, and A channels
        Uint8 r = (Uint8)(startColor.r + t * (endColor.r - startColor.r));
        Uint8 g = (Uint8)(startColor.g + t * (endColor.g - startColor.g));
        Uint8 b = (Uint8)(startColor.b + t * (endColor.b - startColor.b));
        Uint8 a = (Uint8)(startColor.a + t * (endColor.a - startColor.a));

        // 4. Map the color to the surface format and store it
        pixels[y] = SDL_MapSurfaceRGBA(surface, r, g, b, a);
    }
}

void Gradient::RenderGradient(SDL_Renderer* renderer, SDL_Surface* dest)
{
    SDL_BlitSurfaceScaled(m_Surface, NULL, dest, &m_Rect, SDL_SCALEMODE_NEAREST);
}

SDL_Color TempToColor(float temp)
{
    struct TempAndCol
    {
        float m_Temp;
        float m_Hue;
        float m_Saturation;
        float m_Lightness;
    };

    static TempAndCol tempToCol[] = {
        { -10.0f, 257.0f, 83.0f  / 100.0f, 25.0f / 100.0f},
        { 0.0f,   198.0f, 100.0f / 100.0f, 50.0f / 100.0f},
        { 10.0f,  180.0f, 100.0f / 100.0f, 77.0f / 100.0f },
        { 20.0f,  59.0f,  100.0f / 100.0f, 50.0f / 100.0f },
        { 40.0f,  24.0f,  100.0f / 100.0f, 50.0f / 100.0f}
    };
    constexpr int numTemps = 5;

    // 1. Handle Out-of-Bounds (Clamping)
    if (temp <= tempToCol[0].m_Temp)
        return HSLToSDLColor(tempToCol[0].m_Hue, tempToCol[0].m_Saturation, tempToCol[0].m_Lightness);

    if (temp >= tempToCol[numTemps - 1].m_Temp)
        return HSLToSDLColor(tempToCol[numTemps - 1].m_Hue, tempToCol[numTemps - 1].m_Saturation, tempToCol[numTemps - 1].m_Lightness);

    // 2. Find the two entries to interpolate between
    int i = 0;
    for (i = 0; i < numTemps - 1; ++i) {
        if (temp >= tempToCol[i].m_Temp && temp <= tempToCol[i + 1].m_Temp) {
            break;
        }
    }

    // 3. Linear Interpolation (lerp)
    const TempAndCol& a = tempToCol[i];
    const TempAndCol& b = tempToCol[i + 1];

    // Calculate how far temp is between a and b (0.0 to 1.0)
    // Avoid division by zero if two points have the same temperature
    float t = 0.0f;
    if (b.m_Temp != a.m_Temp) {
        t = (temp - a.m_Temp) / (b.m_Temp - a.m_Temp);
    }

    float h = a.m_Hue + t * (b.m_Hue - a.m_Hue);
    float s = a.m_Saturation + t * (b.m_Saturation - a.m_Saturation);
    float l = a.m_Lightness + t * (b.m_Lightness - a.m_Lightness);

    // 4. Convert the interpolated HSL to SDL_Color
    return HSLToSDLColor(h, s, l);
}

char buff[128];

struct WeatherSat
{
    Gradient m_tempGradient;
    const WeatherData* m_WeatherData;
    FlipBookImage m_satPlanet;

    void InitWeatherSat(const WeatherData& weather, SDL_Renderer* ren)
    {
        m_satPlanet.InitFlipbook("planet.bmp", 5, 6, 13, 250);
        m_tempGradient.InitGradient(ren, TempToColor(weather.m_TempMax), TempToColor(weather.m_TempMin), SDL_Rect{ 16, 266, 4, 91 });
        m_WeatherData = &weather;
    }
    void DrawWeatherSat(SDL_Renderer* ren, SDL_Surface* dest, Font& font);
    void OnWeatherUpdated(SDL_Renderer* ren)
    {
        m_tempGradient.InitGradient(ren, TempToColor(m_WeatherData->m_TempMax), TempToColor(m_WeatherData->m_TempMin), m_tempGradient.m_Rect);
    }
};

void WeatherSat::DrawWeatherSat(SDL_Renderer* ren, SDL_Surface* dest, Font& font)
{
    m_satPlanet.UpdateFlipbook(dest);

    snprintf(buff, sizeof(buff), "%0.1fC", m_WeatherData->m_TempMax);
    draw_text(dest, font, 15, 255, buff, 1);

    snprintf(buff, sizeof(buff), "%0.1fC", m_WeatherData->m_TempMin);
    draw_text(dest, font, 15, 360, buff, 1);

    if (m_WeatherData->m_TempMin == m_WeatherData->m_TempMax)
    {
        return;
    }
    m_tempGradient.RenderGradient(ren, dest);

    const float currTempT = 1.0f - (m_WeatherData->m_CurrentTemp - m_WeatherData->m_TempMin) / (m_WeatherData->m_TempMax - m_WeatherData->m_TempMin);
    const int currTempY = (int)((currTempT * m_tempGradient.m_Rect.h) + m_tempGradient.m_Rect.y);
    SDL_Rect currTempRect = { 14, currTempY, 10, 2 };
    SDL_FillSurfaceRect(dest, &currTempRect, SDL_MapSurfaceRGBA(dest, 255, 255, 255, 255));

    snprintf(buff, sizeof(buff), "%0.1fC", m_WeatherData->m_CurrentTemp);
    draw_text(dest, font, 28, currTempY - (font.m_GlyphSurfaceH/3), buff, 1);
}

int main(int argc, char* argv[]) 
{
    srand((unsigned int)time(NULL));

    WeatherData weather;
    UselessFactData uselessFact;

    ReactorCell reactorCells[] = {
        { 18,  399 },
        { 34,  399 },
        { 50,  399 },
        { 66,  399 },
        { 82,  399 },
        { 98,  399 },
        { 114, 399 },
        { 130, 399 },

        { 26,  413 },
        { 42,  413 },
        { 58,  413 },
        { 74,  413 },
        { 90,  413 },
        { 106, 413 },
        { 122, 413 },
        { 138, 413 },

        { 18,  427 },
        { 34,  427 },
        { 50,  427 },
        { 66,  427 },
        { 82,  427 },
        { 98,  427 },
        { 114, 427 },
        { 130, 427 }
    };
    constexpr int numReactorCells = 24;

    std::map<std::string, std::string> values;

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

    SDL_Surface* bg = SDL_LoadBMP("back_ops.bmp");
    SDL_Surface* reactor_red = LoadBMPWithColorKey("reactor_red.bmp", canvas->format);


    SDL_Surface* control_bar = LoadBMPWithColorKey("control_bar.bmp", canvas->format);
    SDL_Surface* control_bar2 = LoadBMPWithColorKey("control_bar2.bmp", canvas->format);
    constexpr int numControlStacks = 5;
    ControlIndicator controlStacks[numControlStacks];
    controlStacks[0].InitControlIndicator(9, 161, 344, *control_bar);
    controlStacks[1].InitControlIndicator(9, 184, 344, *control_bar2);
    controlStacks[2].InitControlIndicator(9, 209, 344, *control_bar);
    controlStacks[3].InitControlIndicator(9, 232, 344, *control_bar2);
    controlStacks[4].InitControlIndicator(9, 256, 344, *control_bar2);

    Font monoFont;
    monoFont.InitFont("font.bmp", 8, 8, canvas->format);

    Font varFont;
    varFont.InitFont("font.bmp", 8, 8, canvas->format);
    varFont.MakeVariableWidth();

    Font fontNumLarge;
    fontNumLarge.InitFont("num_large.bmp", 8, 8, canvas->format);
    fontNumLarge.m_NumbersOnly = true;

    TextWall operationsText;
    operationsText.InitTextWall(160, 170, 140, 140, monoFont);
    //FeedView operationsFeed;

    PanningImage topological;
    topological.InitPanningImage("topological.bmp", 10 + 1, 160, 140 - 2, 70 - 1);

    int reloadValsCnt = 0;
    float reloadValsT = 0.0f;

    constexpr int num_sine = 8;
    PanningImage sines[num_sine];
    for (int i = 0; i < num_sine; ++i)
    {
        PanningImage& sine = sines[i];
        sine.InitPanningImage("sine.bmp", 273, 362, 32, 80);
        sine.m_panHorizontal = false;
        sine.m_Speed = i > 2 ? 1.0f : 2.0f;
        sine.m_Scroll = (float)(i * 4);
        SDL_SetSurfaceColorKey(sine.m_Image, true, SDL_MapSurfaceRGB(sine.m_Image, 0, 0, 0));
    }

    LoadConfigToMap("values.txt", values);

    if (!init_fb())
    {
        return -1;
    }

    bool run = true;
    int frame = 0;

    WeatherSat weatherSat;
    weatherSat.InitWeatherSat(weather, ren);

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

        static const int reloadCntMax = 100;
        if (reloadValsCnt == 0)
        {
            values.clear();
            LoadConfigToMap("values.txt", values);
            if (weather.UpdateWeatherData())
            {
                weatherSat.OnWeatherUpdated(ren);
            }

            if (uselessFact.UpdateUselessFact())
            {

            }
        }
        reloadValsCnt = (reloadValsCnt + 1) % reloadCntMax;
        reloadValsT = (float)reloadValsCnt / (float)reloadCntMax;


        // Draw Background
        if (bg) SDL_BlitSurface(bg, NULL, canvas, NULL);
        else SDL_FillSurfaceRect(canvas, NULL, SDL_MapSurfaceRGBA(canvas, 20, 20, 40, 255));


        for (int i = 0; i < numControlStacks; ++i)
        {
            controlStacks[i].UpdateAndDraw(canvas);
        }

        // Update & Draw Clock Text
        int clock_x = 50;
        int clock_y = 70;
        UpdateClock(clock_x, clock_y, canvas, fontNumLarge);


        snprintf(buff, sizeof(buff), "%s", weather.m_CurrentWeatherDesc.c_str());
        draw_text(canvas, varFont, 50, 110, buff, 1);

        snprintf(buff, sizeof(buff), "Current Temp %0.1fC", weather.m_CurrentTemp);
        draw_text(canvas, varFont, 50, 120, buff, 1);

        //snprintf(buff, sizeof(buff), "%0.1fC Min %0.1fC Max", weather.m_TempMin, weather.m_TempMax);
        //draw_text(canvas, varFont, 20, 115, buff, 2);

        weatherSat.DrawWeatherSat(ren, canvas, varFont);


        static int operationsFeedMode = 0;
        static int operationsFeedModeCnt = 0;
        operationsFeedModeCnt = (operationsFeedModeCnt + 1) % 40;
        if (operationsFeedModeCnt == 0)
        {
            operationsFeedMode = (operationsFeedMode + 1) % 2;
            operationsFeedModeCnt = 0;
        }

        if (operationsFeedMode == 0)
        {
            //operationsFeed.UpdateFeed(operationsText, values);
            operationsText.Clear();
            operationsText.SetWrappedLine(0, uselessFact.m_UselessFact.c_str());
            operationsText.DrawTextWall(canvas, SDL_Color{255, 255, 255, 255});
        }
        else
        {
            static float a = 0.0f, b = 0.0f, c = 0.0f;

            a += 0.2f;
            b += 0.1f;
            c += 0.05f;
            drawSpinningCube(operationsText.m_Text, operationsText.m_CharW, operationsText.m_CharH, a, b, c);

            operationsText.DrawTextWall(canvas, SDL_Color{ 0, 255, 255, 255 });
        }

        topological.UpdateAndDrawPanningImage(ren, canvas);
        for (int i = 0; i < num_sine; ++i)
        {
            sines[i].UpdateAndDrawPanningImage(ren, canvas);
        }

        // Chance of a cell change
        if (rand() % 5 == 0)
        {
            const int reacPosIdx = rand() % numReactorCells;
            reactorCells[reacPosIdx].on = !reactorCells[reacPosIdx].on;
        }

        for (int i = 0; i < numReactorCells; ++i)
        {
            if (!reactorCells[i].on)
            {
                continue;
            }
            SDL_Rect dst = { reactorCells[i].x, reactorCells[i].y, 16, 16 };
            SDL_BlitSurface(reactor_red, NULL, canvas, &dst);
        }

        {
            SDL_SetSurfaceColorMod(reactor_red, (unsigned char)(255 * reloadValsT), (unsigned char)(255 * reloadValsT), (unsigned char)(255 * reloadValsT));
            SDL_Rect dst = { reactorCells[0].x, reactorCells[0].y, 16, 16 };
            SDL_BlitSurface(reactor_red, NULL, canvas, &dst);
            SDL_SetSurfaceColorMod(reactor_red, 255, 255, 255);
        }

        
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

    // TODO more
    SDL_DestroySurface(monoFont.m_Surface);
    SDL_DestroySurface(varFont.m_Surface);
    SDL_DestroySurface(fontNumLarge.m_Surface);
    SDL_Quit();
    return 0;
}