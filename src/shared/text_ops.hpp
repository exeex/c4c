#pragma once

#include <string_view>

#include "text_id_table.hpp"

namespace c4c {

[[nodiscard]] inline TextId intern_text(TextTable& texts,
                                        std::string_view text) {
  return text.empty() ? kInvalidText : texts.intern(text);
}

[[nodiscard]] inline std::string_view lookup_text(const TextTable& texts,
                                                  TextId text_id) {
  return text_id == kInvalidText ? std::string_view{} : texts.lookup(text_id);
}

[[nodiscard]] inline const char* lookup_c_str(const TextTable& texts,
                                              TextId text_id) {
  return text_id == kInvalidText ? "" : texts.c_str(text_id);
}

[[nodiscard]] inline FileId intern_file_path(FileTable& files,
                                             std::string_view path) {
  return files.intern(path);
}

[[nodiscard]] inline std::string_view lookup_file_path(const FileTable& files,
                                                       FileId file_id) {
  return file_id == kInvalidFile ? std::string_view{} : files.lookup(file_id);
}

[[nodiscard]] inline bool text_starts_with(std::string_view text,
                                           std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.compare(0, prefix.size(), prefix) == 0;
}

[[nodiscard]] inline bool text_in_list(std::string_view text,
                                       const char* const* items,
                                       size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (text == items[i]) return true;
  }
  return false;
}

}  // namespace c4c
