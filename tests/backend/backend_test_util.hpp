#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>
#include <iostream>

#include "../../src/codegen/lir/verify.hpp"

inline int& fail_count() { static int c = 0; return c; }
inline void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << "\n";
  ++fail_count();
}
inline void check_failures() {
  if (fail_count() > 0) {
    std::cerr << "\n" << fail_count() << " test(s) FAILED\n";
    std::exit(1);
  }
}

// Optional substring filter set by main() from argv[1]. Empty means run all.
inline std::string& test_filter() { static std::string f; return f; }

#define RUN_TEST(fn) do {   if (test_filter().empty() || std::string(#fn).find(test_filter()) != std::string::npos) {     try { fn(); } catch (const std::exception& e) { fail(std::string(#fn) + " threw: " + e.what()); }   } } while(0)

inline void expect_true(bool condition, const std::string& message = "expectation failed") {
  if (!condition) fail(message);
}

inline void expect_contains(const std::string& text,
                           const std::string& needle,
                           const std::string& message = "") {
  if (text.find(needle) == std::string::npos) {
    fail(message.empty() ? ("Expected to find substring:\n" + needle + "\nActual:\n" + text)
                        : (message + "\nExpected: " + needle + "\nActual:\n" + text));
  }
}

inline void expect_not_contains(const std::string& text,
                               const std::string& needle,
                               const std::string& message = "") {
  if (text.find(needle) != std::string::npos) {
    fail(message.empty() ? ("Unexpected substring:\n" + needle + "\nActual:\n" + text)
                        : (message + "\nUnexpected: " + needle + "\nActual:\n" + text));
  }
}

template <typename Fn>
inline void expect_throws_lir_verify(Fn&& fn,
                              const std::string& needle,
                              const std::string& message = "") {
  try {
    fn();
  } catch (const c4c::codegen::lir::LirVerifyError& ex) {
    expect_true(ex.kind() == c4c::codegen::lir::LirVerifyErrorKind::Malformed,
                message.empty() ? "Expected malformed verifier error kind"
                                : (message + "\nExpected malformed verifier error kind"));
    expect_contains(ex.what(), needle, message);
    return;
  }
  fail(message.empty() ? "Expected LIR verifier throw" : (message + "\nExpected LIR verifier throw"));
}


// --- Utility functions (shared across test files) ---

inline void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value & 0xff));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

inline void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

inline void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int shift = 0; shift < 64; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

inline void append_i64(std::vector<std::uint8_t>& out, std::int64_t value) {
  append_u64(out, static_cast<std::uint64_t>(value));
}

inline void append_zeroes(std::vector<std::uint8_t>& out, std::size_t count) {
  out.insert(out.end(), count, 0);
}

inline std::size_t align_up(std::size_t value, std::size_t alignment) {
  if (alignment == 0) return value;
  const auto mask = alignment - 1;
  return (value + mask) & ~mask;
}

inline std::uint16_t read_u16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return static_cast<std::uint16_t>(bytes[offset]) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

inline std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

inline std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  std::uint64_t value = 0;
  for (int shift = 0; shift < 64; shift += 8) {
    value |= static_cast<std::uint64_t>(bytes[offset + (shift / 8)]) << shift;
  }
  return value;
}

inline std::vector<std::uint8_t> read_elf_entry_bytes(const std::vector<std::uint8_t>& image,
                                               std::size_t byte_count) {
  constexpr std::uint32_t kPtLoad = 1;

  if (image.size() < 64) fail("ELF image too small to read entry bytes");
  const auto entry_address = read_u64(image, 24);
  const auto program_header_offset = read_u64(image, 32);
  const auto program_header_size = read_u16(image, 54);
  const auto program_header_count = read_u16(image, 56);

  for (std::size_t index = 0; index < program_header_count; ++index) {
    const auto header_offset = program_header_offset + (index * program_header_size);
    if (header_offset + program_header_size > image.size()) break;
    if (read_u32(image, header_offset + 0) != kPtLoad) continue;

    const auto file_offset = read_u64(image, header_offset + 8);
    const auto virtual_address = read_u64(image, header_offset + 16);
    const auto file_size = read_u64(image, header_offset + 32);
    if (entry_address < virtual_address || entry_address >= virtual_address + file_size) continue;

    const auto entry_file_offset = file_offset + (entry_address - virtual_address);
    if (entry_file_offset + byte_count > image.size()) {
      fail("ELF entry bytes extend past the file image");
    }
    return std::vector<std::uint8_t>(image.begin() + static_cast<std::ptrdiff_t>(entry_file_offset),
                                     image.begin() + static_cast<std::ptrdiff_t>(entry_file_offset + byte_count));
  }

  fail("ELF image does not map the entry point through a PT_LOAD segment");
}

#if defined(__x86_64__)
inline int execute_x86_64_entry_bytes(const std::vector<std::uint8_t>& entry_bytes) {
  const long page_size = ::sysconf(_SC_PAGESIZE);
  if (page_size <= 0) fail("failed to read host page size for x86 runtime validation");
  const auto mapping_size = align_up(entry_bytes.size(), static_cast<std::size_t>(page_size));
  void* mapping = ::mmap(nullptr, mapping_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED) fail("failed to map executable memory for x86 runtime validation");

  std::copy(entry_bytes.begin(), entry_bytes.end(), static_cast<std::uint8_t*>(mapping));
  const auto fn = reinterpret_cast<int (*)()>(mapping);
  const int result = fn();
  ::munmap(mapping, mapping_size);
  return result;
}
#endif

inline std::uint8_t symbol_info(std::uint8_t binding, std::uint8_t symbol_type) {
  return static_cast<std::uint8_t>((binding << 4) | symbol_type);
}

inline std::string format_ar_field(const std::string& value, std::size_t width) {
  if (value.size() >= width) return value.substr(0, width);
  return value + std::string(width - value.size(), ' ');
}




inline std::size_t find_section_index(const c4c::backend::linker_common::Elf64Object& object,
                               const std::string& name) {
  for (std::size_t index = 0; index < object.sections.size(); ++index) {
    if (object.sections[index].name == name) return index;
  }
  return object.sections.size();
}

inline void write_text_file(const std::string& path, const std::string& text) {
  std::ofstream output(path, std::ios::binary);
  if (!output) fail("failed to open file for write: " + path);
  output << text;
  if (!output.good()) fail("failed to write file: " + path);
}

inline void write_binary_file(const std::string& path, const std::vector<std::uint8_t>& bytes) {
  std::ofstream output(path, std::ios::binary);
  if (!output) fail("failed to open file for write: " + path);
  output.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  if (!output.good()) fail("failed to write file: " + path);
}

inline std::string shell_quote(const std::string& text) {
  std::string quoted = "'";
  for (const char ch : text) {
    if (ch == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(ch);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

inline std::string run_command_capture(const std::string& command) {
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) fail("failed to run command: " + command);

  std::string output;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }

  const int status = pclose(pipe);
  if (status != 0) {
    fail("command failed (" + std::to_string(status) + "): " + command + "\n" + output);
  }
  return output;
}

inline std::string normalize_objdump_surface(const std::string& text) {
  std::stringstream input(text);
  std::string line;
  std::string normalized;
  while (std::getline(input, line)) {
    const auto file_format_pos = line.find("file format ");
    if (file_format_pos != std::string::npos) {
      line = line.substr(file_format_pos);
    }
    normalized += line;
    normalized.push_back('\n');
  }
  return normalized;
}

inline std::string normalize_objdump_disassembly(const std::string& text) {
  std::stringstream input(text);
  std::string line;
  std::string normalized;
  while (std::getline(input, line)) {
    const auto file_format_pos = line.find("file format ");
    if (file_format_pos != std::string::npos) continue;

    const auto colon_pos = line.find(':');
    if (colon_pos != std::string::npos && colon_pos < 18) {
      const auto asm_pos = line.find_first_not_of(" \t", colon_pos + 1);
      if (asm_pos != std::string::npos) {
        line = line.substr(asm_pos);
      }
    }

    normalized += line;
    normalized.push_back('\n');
  }
  return normalized;
}
