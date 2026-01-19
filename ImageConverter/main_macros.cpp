#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <filesystem>

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Surface* source = SDL_LoadSurface(argv[1]);
    SDL_Surface* target = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGB565);

    uint16_t* pixels = (uint16_t*)target->pixels;
    int pixelCount = target->w * target->h;

    // 1. Count color frequency
    std::map<uint16_t, int> counts;
    for (int i = 0; i < pixelCount; i++) counts[pixels[i]]++;

    // 2. Sort to find the most common colors
    std::vector<std::pair<uint16_t, int>> sorted;
    for (auto const& [color, count] : counts) sorted.push_back({ color, count });
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.second > b.second; });

    std::filesystem::path p(argv[1]);
    std::string varName = p.stem().string();
    std::ofstream outFile(varName + ".h");

    outFile << "#include <pgmspace.h>\n\n";

    // 3. Define Macros for the top 26 most frequent colors
    std::map<uint16_t, std::string> macroMap;
    for (int i = 0; i < std::min((int)sorted.size(), 26); i++) {
        std::string mName = "C_" + std::string(1, 'A' + i);
        uint16_t c = sorted[i].first;
        // Split 16-bit color into two hex bytes for the macro
        outFile << "#define " << mName << " \"\\x"
            << std::hex << std::setw(2) << std::setfill('0') << (c & 0xFF)
            << "\\x" << std::setw(2) << std::setfill('0') << (c >> 8) << "\"\n";
        macroMap[c] = mName;
    }

    outFile << "\nconst uint16_t " << varName << "_width  = " << target->w << ";\n";
    outFile << "const uint16_t " << varName << "_height = " << target->h << ";\n\n";
    outFile << "const char " << varName << "_data[] PROGMEM = \n";

    // 4. Write the data using macros where possible
    for (int i = 0; i < pixelCount; i++) {
        if (i % 8 == 0) outFile << (i == 0 ? "" : "\n") << "  ";

        if (macroMap.count(pixels[i])) {
            outFile << macroMap[pixels[i]] << " ";
        }
        else {
            outFile << "\"\\x" << std::hex << std::setw(2) << std::setfill('0') << (pixels[i] & 0xFF)
                << "\\x" << std::setw(2) << std::setfill('0') << (pixels[i] >> 8) << "\" ";
        }
    }
    outFile << ";\n";

    SDL_DestroySurface(source); SDL_DestroySurface(target); SDL_Quit();
    return 0;
}