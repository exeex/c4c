#ifndef TINYC2LL_FRONTEND_CXX_PP_INCLUDE_HPP
#define TINYC2LL_FRONTEND_CXX_PP_INCLUDE_HPP

#include <string>
#include <vector>

namespace c4c {

// Extract the directory component of a file path.
std::string dirname_of(const std::string& path);

// Read an entire file into *out. Returns false on failure.
bool read_file(const std::string& path, std::string* out);

// Shell-quote a string for safe command-line use.
std::string shell_quote(const std::string& s);

// Run a command and capture its stdout into out_text. Returns false on failure.
bool run_capture(const std::string& cmd, std::string& out_text);

// Attempt to preprocess a file using an external system preprocessor
// (ccc, cc, or cpp). Returns empty string on failure.
std::string preprocess_external(const std::string& path);

}  // namespace c4c

#endif
