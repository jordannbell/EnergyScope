#include "energy/FileIo.hpp"

#include <fstream>
#include <stdexcept>

namespace energy {

void writeBinaryFile(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("cannot create file: " + path.string());
    }
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    file.close();  // flushes; a full disk shows up here
    if (!file) {
        throw std::runtime_error("cannot write file: " + path.string());
    }
}

}  // namespace energy
