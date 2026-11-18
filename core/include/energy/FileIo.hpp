#pragma once

#include <filesystem>
#include <string>

namespace energy {

// Writes `bytes` to `path` exactly as given (binary mode: no newline
// translation, which would corrupt a PDF). Replaces an existing file.
// Throws std::runtime_error if the file cannot be created or written.
void writeBinaryFile(const std::filesystem::path& path, const std::string& bytes);

}  // namespace energy
