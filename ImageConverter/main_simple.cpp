#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: img2str <input_file>\n";
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;

    SDL_Surface* source = SDL_LoadSurface(argv[1]);
    if (!source) return 1;

    // Convert to RGB565
    SDL_Surface* target = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGB565);
    if (!target) return 1;

    std::filesystem::path p(argv[1]);
    std::string varName = p.stem().string();
    std::ofstream outFile(varName + ".h");

    outFile << "#include <pgmspace.h>\n\n";
    outFile << "const uint16_t " << varName << "_width  = " << target->w << ";\n";
    outFile << "const uint16_t " << varName << "_height = " << target->h << ";\n\n";

    // We define this as a char array to store the string literal
    outFile << "const char " << varName << "_data[] PROGMEM = \n\"";

    uint8_t* rawBytes = (uint8_t*)target->pixels;
    size_t totalBytes = (size_t)target->w * target->h * 2;

    for (size_t i = 0; i < totalBytes; i++) {
        outFile << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)rawBytes[i];

        // Break lines every 16 bytes so the header file is readable and 
        // doesn't hit compiler line-length limits
        if ((i + 1) % 16 == 0 && (i + 1) < totalBytes) {
            outFile << "\"\n\"";
        }
    }

    outFile << "\";\n";

    SDL_DestroySurface(source);
    SDL_DestroySurface(target);
    SDL_Quit();
    return 0;
}