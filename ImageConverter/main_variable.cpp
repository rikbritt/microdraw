#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <filesystem>
#include <algorithm>

struct Chunk {
    std::vector<uint8_t> data;
    long score;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: imgopt <file>\n";
        return 1;
    }
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Surface* source = SDL_LoadSurface(argv[1]);
    if (!source) return 1;

    // Convert to RGB565
    SDL_Surface* target = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGB565);
    uint8_t* pixels = (uint8_t*)target->pixels;
    size_t totalBytes = (size_t)target->w * target->h * 2;
    std::vector<uint8_t> fullData(pixels, pixels + totalBytes);

    // 1. Pattern Analysis - Identify sequences that repeat often
    std::map<std::vector<uint8_t>, int> chunkCounts;
    for (size_t len = 6; len <= 32; len += 2) {
        for (size_t i = 0; i <= totalBytes - len; i += 2) {
            std::vector<uint8_t> sub(fullData.begin() + i, fullData.begin() + i + len);
            chunkCounts[sub]++;
        }
    }

    std::vector<Chunk> candidates;
    for (auto const& [data, count] : chunkCounts) {
        if (count < 3) continue;
        long savings = (data.size() * 4 - 4) * count;
        candidates.push_back({ data, savings });
    }
    std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) { return a.score > b.score; });

    // Select top 64 patterns for macro optimization
    std::vector<Chunk> selected;
    for (auto& c : candidates) {
        if (selected.size() >= 64) break;
        selected.push_back(c);
    }

    std::filesystem::path p(argv[1]);
    std::string varName = p.stem().string();
    std::ofstream outFile(varName + ".h");

    outFile << "#ifndef _" << varName << "_H_\n";
    outFile << "#define _" << varName << "_H_\n\n";
    outFile << "#include <pgmspace.h>\n\n";

    // 2. Define Macros
    std::map<std::vector<uint8_t>, std::string> macroMap;
    std::vector<std::string> macroNames;
    for (size_t i = 0; i < selected.size(); i++) {
        std::string mName = "M" + std::to_string(i); // Prefixed to ensure uniqueness
        macroMap[selected[i].data] = mName;
        macroNames.push_back(mName);

        outFile << "#define " << mName << " \"";
        for (uint8_t b : selected[i].data)
            outFile << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        outFile << "\"\n";
    }

    outFile << "\nconst uint16_t " << varName << "_width  = " << target->w << ";\n";
    outFile << "const uint16_t " << varName << "_height = " << target->h << ";\n\n";
    outFile << "const char " << varName << "_data[] PROGMEM = \n  ";

    // 3. Optimized Output Generation
    std::string rawBuffer = "";
    auto flushBuffer = [&]() {
        if (!rawBuffer.empty()) {
            outFile << "\"" << rawBuffer << "\" ";
            rawBuffer = "";
        }
        };

    for (size_t i = 0; i < totalBytes; ) {
        bool replaced = false;
        for (auto const& sel : selected) {
            if (i + sel.data.size() <= totalBytes &&
                std::equal(sel.data.begin(), sel.data.end(), fullData.begin() + i)) {

                flushBuffer();
                outFile << macroMap[sel.data] << " ";
                i += sel.data.size();
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            char hex[5];
            sprintf(hex, "\\x%02x", fullData[i]);
            rawBuffer += hex;
            i++;
            if (rawBuffer.length() > 120) flushBuffer();
        }

        if (i % 32 == 0) outFile << "\n  ";
    }

    flushBuffer();
    outFile << ";\n\n";

    // 4. Undefine Macros to keep namespace clean
    outFile << "// Cleanup macros\n";
    for (const auto& mName : macroNames) {
        outFile << "#undef " << mName << "\n";
    }

    outFile << "\n#endif\n";

    SDL_DestroySurface(source); SDL_DestroySurface(target); SDL_Quit();
    std::cout << "Header generated: " << varName << ".h" << std::endl;
    return 0;
}