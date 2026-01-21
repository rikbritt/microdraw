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

extern bool md_init_impl(int width, int height);
extern void md_deinit_impl();
extern MD_Image* md_load_image_impl(const char* filename);
extern MD_Image* md_load_image_with_key_impl(const char* filename, uint8_t key_r, uint8_t key_g, uint8_t key_b);
extern MD_Image* md_create_image_impl(int w, int h);
extern void md_draw_pixel_to_image_impl(MD_Image& image, int x, int y, uint8_t r, uint8_t g, uint8_t b);
extern void md_destroy_image_impl(MD_Image& image);
extern int md_get_image_width_impl(const MD_Image& image);
extern int md_get_image_height_impl(const MD_Image& image);
extern bool md_draw_image_impl(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect);
extern bool md_draw_image_scaled_impl(MD_Image& image, MD_Rect* srcRect, MD_Image* dest, MD_Rect* destRect);
extern void md_filled_rect_impl(MD_Rect& rect, uint8_t r, uint8_t g, uint8_t b);
extern void md_set_image_clip_impl(MD_Image& image, MD_Rect* rect);
extern void md_set_colour_mod_impl(MD_Image& image, uint8_t key_r, uint8_t key_g, uint8_t key_b);
extern void md_set_clip_impl(MD_Rect* rect);
extern void md_get_pixel_x_bounds_impl(MD_Image& image, const MD_Rect& rect, int& xLeftOut, int& xRightOut);
extern void md_render_impl();
extern bool md_exit_raised_impl();

bool md_init(int width, int height)
{
	return md_init_impl(width, height);
}

void md_deinit()
{
	md_deinit_impl();
}

MD_Image* md_load_image(const char* filename)
{
	return md_load_image_impl(filename);
}

MD_Image* md_load_image_with_key(const char* filename, uint8_t key_r, uint8_t key_g, uint8_t key_b)
{
	return md_load_image_with_key_impl(filename, key_r, key_g, key_b);
}

MD_Image* md_create_image(int w, int h)
{
	return md_create_image_impl(w, h);
}

void md_destroy_image(MD_Image& image)
{
	md_destroy_image_impl(image);
}

void md_draw_pixel_to_image(MD_Image& image, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	md_draw_pixel_to_image_impl(image, x, y, r, g, b);
}

int md_get_image_width(const MD_Image& image)
{
	return md_get_image_width_impl(image);
}

int md_get_image_height(const MD_Image& image)
{
	return md_get_image_height_impl(image);
}

bool md_draw_image(MD_Image& image)
{
	return md_draw_image_impl(image, nullptr, nullptr, nullptr);
}

bool md_draw_image(MD_Image& image, int x, int y)
{
	MD_Rect destRect{ x, y, md_get_image_width(image), md_get_image_height(image) };
	return md_draw_image_impl(image, nullptr, nullptr, &destRect);
}

bool md_draw_image(MD_Image& image, MD_Rect& src, MD_Rect& dest)
{
	return md_draw_image_impl(image, &src, nullptr, &dest);
}

bool md_draw_image_scaled(MD_Image& image, MD_Rect& src, MD_Rect& dest)
{
	return md_draw_image_scaled_impl(image, &src, nullptr, &dest);
}

bool md_draw_image_scaled(MD_Image& image, MD_Rect& dest)
{
	return md_draw_image_scaled_impl(image, nullptr, nullptr, &dest);
}

void Font::InitFont(const char* bmpName, int w, int h)
{
	m_GlyphSurfaceW = w;
	m_GlyphSurfaceH = h;
	m_Surface = md_load_image_with_key(bmpName, 0, 0, 0);
}

void md_filled_rect(MD_Rect& rect, uint8_t r, uint8_t g, uint8_t b)
{
	md_filled_rect_impl(rect, r, g, b);
}

void md_set_image_clip(MD_Image& image, MD_Rect& rect)
{
	md_set_image_clip_impl(image, &rect);
}

void md_set_clip(MD_Rect& rect)
{
	md_set_clip_impl(&rect);
}

void md_clear_clip()
{
	md_set_clip_impl(nullptr);
}

void md_set_colour_mod(MD_Image& image, uint8_t key_r, uint8_t key_g, uint8_t key_b)
{
	md_set_colour_mod_impl(image, key_r, key_g, key_b);
}

void md_render()
{
	md_render_impl();
}

bool md_exit_raised()
{
	return md_exit_raised_impl();
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

MD_Rect Font::GetGlpyphRect(char c) const
{
	// Calculate position in a 16x8 grid (Standard ASCII layout)
	const int w = GetGlyphWidth(c);
	const int h = GetGlyphHeight(c);
	MD_Rect src = { (c % 16) * m_GlyphSurfaceW, (c / 16) * m_GlyphSurfaceH, w, h };
	if (!m_Monospace)
	{
		src.x += m_GlyphData[c].left;
	}
	return src;
}

void Font::MakeVariableWidth()
{
	for (int i = 0; i < 256; ++i)
	{
		const char c = (char)i;
		const int w = m_GlyphSurfaceW;
		const int h = m_GlyphSurfaceH;
		const MD_Rect src = GetGlpyphRect(c);

		// Get where the font pixel data starts and stops along the X axis, and set the character width to match
		GlyphData& glyphData = m_GlyphData[i];
		md_get_pixel_x_bounds_impl(*m_Surface, src, glyphData.left, glyphData.right);
		//GetPixelXBounds(m_Surface, src, glyphData.left, glyphData.right);
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

// Draw text using an 8x8 bitmap font sheet
void draw_text(Font& font, int x, int y, const char* text, int scale)
{
	MD_Rect dst = { x, y, 0, 0 };
	for (int i = 0; text[i] != '\0'; i++)
	{
		int ascii = (unsigned char)text[i] - 1;
		MD_Rect src = font.GetGlpyphRect(ascii);
		dst.w = src.w * scale;
		dst.h = src.h * scale;
		//SDL_Rect src = { (ascii % 16) * 8, (ascii / 16) * 8, 8, 8 };
		//SDL_Rect dst = { x + (i * 8 * scale), y, 8 * scale, 8 * scale };
		md_draw_image_scaled(*font.m_Surface, src, dst);
		dst.x += src.w * scale;
		dst.x += font.m_SpacingX * scale;
	}
}

void draw_num(Font& font, int x, int y, const char* text, int scale)
{
	const int glyph_width = font.m_GlyphSurfaceW;
	const int glyph_height = font.m_GlyphSurfaceH;
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
		MD_Rect src = { xIdx * glyph_width, yIdx * glyph_height, glyph_width, glyph_height };
		MD_Rect dst = { x + (i * (glyph_width + space_x) * scale), y, glyph_width * scale, glyph_height * scale };
		md_draw_image_scaled(*font.m_Surface, src, dst);
		//int res = SDL_BlitSurfaceScaled(font.m_Surface, &src, dest, &dst, SDL_SCALEMODE_NEAREST);
		//if (res != 0)
		//{
		//    const char* err = SDL_GetError();
		//}
	}
}




void PanningImage::InitPanningImage(const char* file, int x, int y, int w, int h)
{
	m_Image = md_load_image_with_key(file, 0, 0, 0);
	m_Rect.x = x;
	m_Rect.y = y;
	m_Rect.w = w;
	m_Rect.h = h;
}

void PanningImage::UpdateAndDrawPanningImage()
{
	md_set_clip(m_Rect);
	MD_Rect scrolled = m_Rect;
	if (m_panHorizontal)
	{
		scrolled.x += (int)m_Scroll;
	}
	else
	{
		scrolled.y += (int)m_Scroll;
	}
	m_Scroll = m_Scroll + m_Speed;
	md_draw_image(*m_Image, scrolled.x, scrolled.y);
	//SDL_BlitSurface(m_Image, NULL, dest, &scrolled);
	if (m_panHorizontal)
	{
		const int w = md_get_image_width(*m_Image);
		scrolled.x -= w;
		if (m_Scroll > w)
		{
			m_Scroll -= w;
		}
	}
	else
	{
		const int h = md_get_image_height(*m_Image);
		scrolled.y -= h;
		if (m_Scroll > h)
		{
			m_Scroll -= h;
		}
	}
	//SDL_BlitSurface(m_Image, NULL, dest, &scrolled);
	md_draw_image(*m_Image, scrolled.x, scrolled.y);

	md_clear_clip();
}



void FlipBookImage::InitFlipbook(const char* bmpName, int numCols, int numRows, int x, int y)
{
	m_Image = md_load_image(bmpName);
	m_Width = md_get_image_width(*m_Image) / numCols;
	m_Height = md_get_image_height(*m_Image) / numRows;
	m_Cols = numCols;
	m_Rows = numRows;
	m_X = x;
	m_Y = y;
}

void FlipBookImage::UpdateFlipbook()
{
	const int sourceIdxX = (m_Frame % m_Cols);
	const int sourceIdxY = (m_Frame / m_Cols);
	m_Frame = (m_Frame + 1) % (m_Cols * m_Rows);
	MD_Rect sourceRect = { sourceIdxX * m_Width, sourceIdxY * m_Height, m_Width, m_Height };
	MD_Rect destRect = { m_X, m_Y, m_Width, m_Height };
	md_draw_image(*m_Image, sourceRect, destRect);
	//SDL_BlitSurface(m_Image, &sourceRect, dest, &destRect);
}








// Internal helper to skip whitespace
void SkipWhitespace(const char*& json) {
	while (*json && std::isspace(*json)) json++;
}

// Forward declaration
bool ParseValue(const char*& json, JSONVal& node);

// Helper to parse strings (and numbers as strings)
bool ParseString(const char*& json, std::string& out) 
{
	if (*json != '\"')
	{
		return false;
	}

	json++; // skip opening quote
	while (*json && *json != '\"') 
	{
		// Detect escaped character
		if (*json == '\\')
		{
			// Move along to the escaped character. It will
			// be copied to the string.
			json++;
		}
		out += *json++;
	}
	if (*json == '\"')
	{
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

	if (*json == '{') // Object
	{
		node.m_object = new std::map<std::string, JSONVal*>();
		json++;
		while (*json && *json != '}')
		{
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

bool ParseJSON(const char* json, JSONVal& jsonDocOut)
{
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
MD_Color HSLToSDLColor(float h, float s, float l, uint8_t a)
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

	MD_Color color;
	color.r = static_cast<uint8_t>((r_tmp + m) * 255);
	color.g = static_cast<uint8_t>((g_tmp + m) * 255);
	color.b = static_cast<uint8_t>((b_tmp + m) * 255);
	color.a = a;

	return color;
}

int LerpInt(float t, int from, int to)
{
	const int out = ((to - from) * t) + from;
	return out;
}



/**
 * Loads key=value pairs into an existing map.
 * @param filename The path to the file as a C-string.
 * @param configMap Reference to the map where data will be stored.
 */
void LoadConfigToMap(const char* filename, Values& configMap)
{
	std::ifstream file(filename);

	if (!file.is_open())
	{
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
