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

void Font::InitFont(const char* bmpName, int w, int h, SDL_PixelFormat format)
{
    m_GlyphSurfaceW = w;
    m_GlyphSurfaceH = h;

    SDL_Surface* temp_font = SDL_LoadBMP(bmpName);
    // Force the font into the EXACT same format as our canvas (32-bit ARGB/XRGB)
    m_Surface = SDL_ConvertSurface(temp_font, format);
    SDL_DestroySurface(temp_font);

    // Re-apply transparency on the NEW surface
    SDL_SetSurfaceColorKey(m_Surface, true, SDL_MapSurfaceRGB(m_Surface, 0, 0, 0));
}

int Font::GetGlyphWidth(char c) const
{
    if (m_Monospace)
    {
        return m_GlyphSurfaceW;
    }

    return m_GlyphData[c].width;
}

int Font::GetGlyphHeight(char c) const 
{
    return m_GlyphSurfaceH;
}

SDL_Rect Font::GetGlpyphRect(char c) const 
{
    // Calculate position in a 16x8 grid (Standard ASCII layout)
    const int w = GetGlyphWidth(c);
    const int h = GetGlyphHeight(c);
    SDL_Rect src = { (c % 16) * m_GlyphSurfaceW, (c / 16) * m_GlyphSurfaceH, w, h };
    if (!m_Monospace)
    {
        src.x += m_GlyphData[c].left;
    }
    return src;
}

void GetPixelXBounds(SDL_Surface* surface, SDL_Rect rect, int& xLeftOut, int& xRightOut) 
{
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

void Font::MakeVariableWidth()
{
    for (int i = 0; i < 256; ++i)
    {
        const char c = (char)i;
        const int w = m_GlyphSurfaceW;
        const int h = m_GlyphSurfaceH;
        const SDL_Rect src = GetGlpyphRect(c);

        // Get where the font pixel data starts and stops along the X axis, and set the character width to match
        GlyphData& glyphData = m_GlyphData[i];
        GetPixelXBounds(m_Surface, src, glyphData.left, glyphData.right);
        glyphData.width = (glyphData.right - glyphData.left) + 1;

        // For fully empty characters
        if (glyphData.width <= 0)
        {
            glyphData.width = w / 2;
        }
     
    }

    m_Monospace = false;
    m_SpacingX = 1;
}


typedef struct {
    int fb_fd;
    unsigned short* fbp;
    int xres;
} FBInfo;

SDL_Surface* LoadBMPWithColorKey(const char* bmpName, SDL_PixelFormat format)
{
    SDL_Surface* temp = SDL_LoadBMP(bmpName);

    // Force the bmp into the EXACT same format as our canvas (32-bit ARGB/XRGB)
    SDL_Surface* bmp = SDL_ConvertSurface(temp, format);
    SDL_DestroySurface(temp);

    // Re-apply transparency on the NEW surface
    SDL_SetSurfaceColorKey(bmp, true, SDL_MapSurfaceRGB(bmp, 0, 0, 0));
    return bmp;
}


/**
 * Loads key=value pairs into an existing map.
 * @param filename The path to the file as a C-string.
 * @param configMap Reference to the map where data will be stored.
 */
void LoadConfigToMap(const char* filename, Values& configMap)
{
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // Skip empty lines to prevent errors
        if (line.empty()) continue;

        size_t delimiterPos = line.find('=');

        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            configMap[key] = value;
        }
    }

    file.close();
}

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
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            Uint32 c = p[y * SCREEN_WIDTH + x];
            fb_info.fbp[y * fb_info.xres + x] = ((c >> 19) << 11) | (((c >> 10) & 0x3F) << 5) | (c >> 3 & 0x1F);
        }
    }
#else
    SDL_Delay(1000 / FB_FPS_LIMIT);
#endif
}

// Draw text using an 8x8 bitmap font sheet
void draw_text(SDL_Surface* dest, Font& font, int x, int y, const char* text, int scale)
{
    SDL_Rect dst = { x, y, 0, 0 };
    for (int i = 0; text[i] != '\0'; i++)
    {
        int ascii = (unsigned char)text[i] - 1;
        SDL_Rect src = font.GetGlpyphRect(ascii);
        dst.w = src.w * scale;
        dst.h = src.h * scale;
        //SDL_Rect src = { (ascii % 16) * 8, (ascii / 16) * 8, 8, 8 };
        //SDL_Rect dst = { x + (i * 8 * scale), y, 8 * scale, 8 * scale };
        SDL_BlitSurfaceScaled(font.m_Surface, &src, dest, &dst, SDL_SCALEMODE_NEAREST);
        dst.x += src.w * scale;
        dst.x += font.m_SpacingX * scale;
    }
}

void draw_num(SDL_Surface* dest, Font& font, int x, int y, const char* text, int scale)
{
    int glyph_width = 14;
    int glyph_height = 15;
    int space_x = 2;

    for (int i = 0; text[i] != '\0'; i++)
    {
        int ascii = (unsigned char)text[i] - 1;
        if (ascii < 47 || ascii > 56)
        {
            continue;
        }
        ascii -= 47;

        // Calculate position in a 16x8 grid (Standard ASCII layout)
        int xIdx = ascii % 5;
        int yIdx = ascii / 5;
        SDL_Rect src = { xIdx * glyph_width, yIdx * glyph_height, glyph_width, glyph_height };
        SDL_Rect dst = { x + (i * (glyph_width + space_x) * scale), y, glyph_width * scale, glyph_height * scale };
        int res = SDL_BlitSurfaceScaled(font.m_Surface, &src, dest, &dst, SDL_SCALEMODE_NEAREST);
        if (res != 0)
        {
            const char* err = SDL_GetError();
        }
    }
}



void FlipBookImage::InitFlipbook(const char* bmpName, int numCols, int numRows, int x, int y)
{
    m_Image = SDL_LoadBMP(bmpName);
    m_Width = m_Image->w / numCols;
    m_Height = m_Image->h / numRows;
    m_Cols = numCols;
    m_Rows = numRows;
    m_X = x;
    m_Y = y;
}

void FlipBookImage::UpdateFlipbook(SDL_Surface* dest)
{
    const int sourceIdxX = (m_Frame % m_Cols);
    const int sourceIdxY = (m_Frame / m_Cols);
    m_Frame = (m_Frame + 1) % (m_Cols * m_Rows);
    const SDL_Rect sourceRect = { sourceIdxX * m_Width, sourceIdxY * m_Height, m_Width, m_Height };
    SDL_Rect destRect = { m_X, m_Y, m_Width, m_Height };
    SDL_BlitSurface(m_Image, &sourceRect, dest, &destRect);
}






void PanningImage::InitPanningImage(const char* file, int x, int y, int w, int h)
{
    m_Image = SDL_LoadBMP(file);
    m_Rect.x = x;
    m_Rect.y = y;
    m_Rect.w = w;
    m_Rect.h = h;
}

void PanningImage::UpdateAndDrawPanningImage(SDL_Renderer* ren, SDL_Surface* dest)
{
    SDL_SetSurfaceClipRect(dest, &m_Rect);
    SDL_Rect scrolled = m_Rect;
    if (m_panHorizontal)
    {
        scrolled.x += (int)m_Scroll;
    }
    else
    {
        scrolled.y += (int)m_Scroll;
    }
    m_Scroll = m_Scroll + m_Speed;
    SDL_BlitSurface(m_Image, NULL, dest, &scrolled);
    if (m_panHorizontal)
    {
        scrolled.x -= m_Image->w;
        if (m_Scroll > m_Image->w)
        {
            m_Scroll -= m_Image->w;
        }
    }
    else
    {
        scrolled.y -= m_Image->h;
        if (m_Scroll > m_Image->h)
        {
            m_Scroll -= m_Image->h;
        }
    }
    SDL_BlitSurface(m_Image, NULL, dest, &scrolled);

    SDL_SetSurfaceClipRect(dest, NULL);
}







// Internal helper to skip whitespace
void SkipWhitespace(const char*& json) {
    while (*json && std::isspace(*json)) json++;
}

// Forward declaration
bool ParseValue(const char*& json, JSONVal& node);

// Helper to parse strings (and numbers as strings)
bool ParseString(const char*& json, std::string& out) {
    if (*json != '\"') return false;
    json++; // skip opening quote
    while (*json && *json != '\"') {
        out += *json++;
    }
    if (*json == '\"') {
        json++; // skip closing quote
        return true;
    }
    return false;
}

// Helper to parse raw numbers as strings per your requirement
bool ParseNumberAsString(const char*& json, std::string& out) {
    const char* start = json;
    if (*json == '-') json++;
    while (*json && (std::isdigit(*json) || *json == '.' || *json == 'e' || *json == 'E' || *json == '+' || *json == '-')) {
        json++;
    }
    if (json == start) return false;
    out = std::string(start, json - start);
    return true;
}

bool ParseValue(const char*& json, JSONVal& node) {
    SkipWhitespace(json);

    if (*json == '{') { // Object
        node.m_object = new std::map<std::string, JSONVal*>();
        json++;
        while (*json && *json != '}') {
            SkipWhitespace(json);
            std::string key;
            if (!ParseString(json, key)) return false;

            SkipWhitespace(json);
            if (*json != ':') return false;
            json++; // skip ':'

            JSONVal* child = new JSONVal();
            if (!ParseValue(json, *child)) return false;
            (*node.m_object)[key] = child;

            SkipWhitespace(json);
            if (*json == ',') json++;
        }
        if (*json == '}') { json++; return true; }
    }
    else if (*json == '[') { // Array
        node.m_array = new std::vector<JSONVal*>();
        json++;
        while (*json && *json != ']') {
            JSONVal* child = new JSONVal();
            if (!ParseValue(json, *child)) return false;
            node.m_array->push_back(child);

            SkipWhitespace(json);
            if (*json == ',') json++;
        }
        if (*json == ']') { json++; return true; }
    }
    else if (*json == '\"') { // String
        node.m_value = new std::string();
        return ParseString(json, *node.m_value);
    }
    else if (std::isdigit(*json) || *json == '-') { // Number
        node.m_value = new std::string();
        return ParseNumberAsString(json, *node.m_value);
    }

    return false;
}

bool ParseJSON(const char* json, JSONVal& jsonDocOut) {
    jsonDocOut.Reset();
    if (!json) return false;
    return ParseValue(json, jsonDocOut);
}

/**
 * Loads a file from disk and parses it into the provided JSONVal structure.
 * Works on Windows, Linux, and macOS.
 */
bool ParseJSONFile(const char* filename, JSONVal& jsonDocOut)
{
    jsonDocOut.Reset();

    // Open the file in input mode
    std::ifstream file(filename);

    if (!file.is_open()) {
        return false; // File not found or access denied
    }

    // Use a stringstream to read the entire file content into a string buffer
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Close the file explicitly (though ifstream destructor handles this too)
    file.close();

    // Check if file was empty
    if (content.empty()) {
        return false;
    }

    // Call your existing parser
    return ParseJSON(content.c_str(), jsonDocOut);
}

JSONVal JSONVal::Invalid;

void JSONVal::Reset()
{
    delete m_value;
    if (m_object)
    {
        for (auto const& [key, val] : *m_object) delete val;
        delete m_object;
        m_object = nullptr;
    }
    if (m_array)
    {
        for (auto* item : *m_array) delete item;
        delete m_array;
        m_array = nullptr;
    }
}

const JSONVal& JSONVal::GetObject(const char* name) const
{
    if (m_object == nullptr)
    {
        return Invalid;
    }
    if (!m_object->contains(name))
    {
        return Invalid;
    }

    return *m_object->find(name)->second;
}

const JSONVal& JSONVal::GetArrayVal(int index) const
{
    if (m_array == nullptr)
    {
        return Invalid;
    }
    if (index < 0 || index >= m_array->size())
    {
        return Invalid;
    }

    return *m_array->at(index);
}

int JSONVal::GetAsInt() const
{
    if (m_value == nullptr)
    {
        return -1;
    }

    if (m_value->empty())
    {
        return -1;
    }

    size_t processed_char_count = 0;
    int result = std::stoi(*m_value, &processed_char_count);

    // If the number of characters processed by stoi is less than 
    // the string length, the string contained trailing garbage.
    if (processed_char_count != m_value->length()) {
        return -1;
    }

    return result;
}

float JSONVal::GetAsFloat() const
{
    if (m_value == nullptr)
    {
        return 0.0f;
    }

    if (m_value->empty())
    {
        return 0.0f;
    }

    size_t processed_char_count = 0;
    float result = std::stof(*m_value, &processed_char_count);

    // If the number of characters processed by stof is less than 
    // the string length, the string contained trailing garbage.
    if (processed_char_count != m_value->length()) {
        return 0.0f;
    }

    return result;
}

const char* JSONVal::GetAsString() const
{
    if (m_value == nullptr)
    {
        return "";
    }

    return m_value->c_str();
}

/**
 * Converts HSL color values to SDL_Color (RGBA).
 * @param h Hue in degrees [0.0, 360.0]
 * @param s Saturation percentage [0.0, 1.0]
 * @param l Lightness percentage [0.0, 1.0]
 * @param a Alpha value [0, 255] (defaults to 255)
 */
SDL_Color HSLToSDLColor(float h, float s, float l, Uint8 a) 
{
    // Clamp values to ensure they are in range
    h = fmod(h, 360.0f);
    if (h < 0) h += 360.0f;
    s = std::clamp(s, 0.0f, 1.0f);
    l = std::clamp(l, 0.0f, 1.0f);

    float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s; // Chroma
    float x = c * (1.0f - std::abs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    float r_tmp = 0, g_tmp = 0, b_tmp = 0;

    if (h < 60) { r_tmp = c; g_tmp = x; b_tmp = 0; }
    else if (h < 120) { r_tmp = x; g_tmp = c; b_tmp = 0; }
    else if (h < 180) { r_tmp = 0; g_tmp = c; b_tmp = x; }
    else if (h < 240) { r_tmp = 0; g_tmp = x; b_tmp = c; }
    else if (h < 300) { r_tmp = x; g_tmp = 0; b_tmp = c; }
    else { r_tmp = c; g_tmp = 0; b_tmp = x; }

    SDL_Color color;
    color.r = static_cast<Uint8>((r_tmp + m) * 255);
    color.g = static_cast<Uint8>((g_tmp + m) * 255);
    color.b = static_cast<Uint8>((b_tmp + m) * 255);
    color.a = a;

    return color;
}

int LerpInt(float t, int from, int to)
{
    const int out = ((to - from) * t) + from;
    return out;
}