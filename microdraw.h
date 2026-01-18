#pragma once

#include <map>
#include <string>
#include <vector>

struct MD_Image;

struct MD_Rect
{
    int x, y;
    int w, h;
};

struct MD_Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

bool md_init(int width, int height);
void md_deinit();
MD_Image* md_load_image(const char* filename);

// Load image with transparent colour key
MD_Image* md_load_image_with_key(const char* filename, uint8_t key_r, uint8_t key_g, uint8_t key_b);
MD_Image* md_create_image(int w, int h);
void md_destroy_image(MD_Image& image);
void md_draw_pixel_to_image(MD_Image& image, int x, int y, uint8_t r, uint8_t g, uint8_t b);
int md_get_image_width(const MD_Image& image);
int md_get_image_height(const MD_Image& image);
bool md_draw_image(MD_Image& image);
bool md_draw_image(MD_Image& image, int x, int y);
bool md_draw_image(MD_Image& image, MD_Rect& src, MD_Rect& dest);
bool md_draw_image_scaled(MD_Image& image, MD_Rect& src, MD_Rect& dest);
bool md_draw_image_scaled(MD_Image& image, MD_Rect& dest);
void md_filled_rect(MD_Rect& rect, uint8_t r, uint8_t g, uint8_t b);
void md_set_image_clip(MD_Image& image, MD_Rect& rect);
void md_set_clip(MD_Rect& rect);
void md_clear_clip();
void md_set_colour_mod(MD_Image& image, uint8_t key_r, uint8_t key_g, uint8_t key_b);
void md_render();
bool md_exit_raised();

class Font
{
public:
    void InitFont(const char* bmpName, int w, int h);
    void MakeVariableWidth();

    int GetGlyphWidth(char c) const;
    int GetGlyphHeight(char c) const;

    MD_Rect GetGlpyphRect(char c) const;

    MD_Image* m_Surface;
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

void draw_text(Font& font, int x, int y, const char* text, int scale);
void draw_num(Font& font, int x, int y, const char* text, int scale);



class PanningImage
{
public:
    void InitPanningImage(const char* file, int x, int y, int w, int h);

    void UpdateAndDrawPanningImage();

    MD_Image* m_Image;
    MD_Rect m_Rect;
    float m_Speed = 0.25f;
    float m_Scroll = 0;
    bool m_panHorizontal = true;
};



// Display an image built from a sprite sheet / image with many frames.
class FlipBookImage
{
public:
    void InitFlipbook(const char* bmpName, int numCols, int numRows, int x, int y);

    void UpdateFlipbook();

    MD_Image* m_Image = nullptr;

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




/**
 * Converts HSL color values to SDL_Color (RGBA).
 * @param h Hue in degrees [0.0, 360.0]
 * @param s Saturation percentage [0.0, 1.0]
 * @param l Lightness percentage [0.0, 1.0]
 * @param a Alpha value [0, 255] (defaults to 255)
 */
MD_Color HSLToSDLColor(float h, float s, float l, uint8_t a = 255);
int LerpInt(float t, int from, int to);


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




typedef std::map<std::string, std::string> Values;
void LoadConfigToMap(const char* filename, Values& configMap);