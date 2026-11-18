#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>

#include "energy/FileIo.hpp"

namespace {

std::filesystem::path tempFile(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

std::string readAll(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST(WriteBinaryFile, WritesBytesUnchangedIncludingNewlinesAndNul) {
    const auto path = tempFile("energyscope_fileio_test.bin");
    const std::string bytes("a\nb\r\nc\0d\xFF", 9);

    energy::writeBinaryFile(path, bytes);

    EXPECT_EQ(readAll(path), bytes);  // no "\n" -> "\r\n" translation
    std::filesystem::remove(path);
}

TEST(WriteBinaryFile, ReplacesAnExistingFile) {
    const auto path = tempFile("energyscope_fileio_replace.bin");
    energy::writeBinaryFile(path, "a long first content");

    energy::writeBinaryFile(path, "short");

    EXPECT_EQ(readAll(path), "short");
    std::filesystem::remove(path);
}

TEST(WriteBinaryFile, ThrowsWhenTheFolderDoesNotExist) {
    EXPECT_THROW(energy::writeBinaryFile("Z:/no/such/folder/x.pdf", "data"),
                 std::runtime_error);
}
