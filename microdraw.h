#pragma once

#include <map>
#include <string>
#include <vector>

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_pixels.h>

struct SDL_Surface;
struct SDL_Renderer;
class Font;


const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 480;

bool init_fb();
void blit_to_fb(SDL_Surface* surf);
void draw_text(SDL_Surface* dest, Font& font, int x, int y, const char* text, int scale);
void draw_num(SDL_Surface* dest, Font& font, int x, int y, const char* text, int scale);

typedef std::map<std::string, std::string> Values;
void LoadConfigToMap(const char* filename, Values& configMap);

SDL_Surface* LoadBMPWithColorKey(const char* bmpName, SDL_PixelFormat format);

class Font
{
public:
    void InitFont(const char* bmpName, int w, int h, SDL_PixelFormat format);
    void MakeVariableWidth();

    int GetGlyphWidth(char c) const;
    int GetGlyphHeight(char c) const;

    SDL_Rect GetGlpyphRect(char c) const;

    SDL_Surface* m_Surface;
    int m_SpacingX = 0;
    int m_GlyphSurfaceW;
    int m_GlyphSurfaceH;
    int m_NumbersOnly = false;
    bool m_Monospace = true;

    struct GlyphData 
    {
        int left = -1; //x of first non black pixel
        int right = -1; //x of last non black pixel
        int width = 0;
    };
    GlyphData m_GlyphData[256];
};

// Display an image built from a sprite sheet / image with many frames.
class FlipBookImage
{
public:
    void InitFlipbook(const char* bmpName, int numCols, int numRows, int x, int y);

    void UpdateFlipbook(SDL_Surface* dest);

    SDL_Surface* m_Image = nullptr;

    // How many columns and rows are in the flipbook image
    int m_Cols = 0;
    int m_Rows = 0;

    // Where to draw the flipbook image
    int m_X = 0;
    int m_Y = 0;

    // Width and height of a single frame in the flipbook
    int m_Width = 0;
    int m_Height = 0;

    // Current frame index
    int m_Frame = 0;
};



class PanningImage
{
public:
    void InitPanningImage(const char* file, int x, int y, int w, int h);

    void UpdateAndDrawPanningImage(SDL_Renderer* ren, SDL_Surface* dest);

    SDL_Surface* m_Image;
    SDL_Rect m_Rect;
    float m_Speed = 0.25f;
    float m_Scroll = 0;
    bool m_panHorizontal = true;
};


struct JSONVal
{
    ~JSONVal()
    {
        Reset();
    }

    void Reset();

    const JSONVal& GetObject(const char* name) const;
    const JSONVal& GetArrayVal(int index) const;
    int GetAsInt() const;
    float GetAsFloat() const;
    const char* GetAsString() const;

    std::map<std::string, JSONVal*>* m_object = nullptr;
    std::string* m_value = nullptr;
    std::vector<JSONVal*>* m_array = nullptr;

protected:
    static JSONVal Invalid;
};

bool ParseJSON(const char* json, JSONVal& jsonDocOut);
bool ParseJSONFile(const char* filename, JSONVal& jsonDocOut);



/**
 * Converts HSL color values to SDL_Color (RGBA).
 * @param h Hue in degrees [0.0, 360.0]
 * @param s Saturation percentage [0.0, 1.0]
 * @param l Lightness percentage [0.0, 1.0]
 * @param a Alpha value [0, 255] (defaults to 255)
 */
SDL_Color HSLToSDLColor(float h, float s, float l, Uint8 a = 255);
int LerpInt(float t, int from, int to);