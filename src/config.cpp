#include "config.h"
#include <fstream>
#include <filesystem>

void Config::Save(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(this), sizeof(Config));
        file.close();
    }
}

void Config::Load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(this), sizeof(Config));
        file.close();
    }
}
