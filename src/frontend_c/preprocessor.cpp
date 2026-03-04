#include "preprocessor.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tinyc2ll::frontend_cxx {

namespace {

std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream buf;
  buf << in.rdbuf();
  return buf.str();
}

std::string shell_quote(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 2);
  out.push_back('\'');
  for (char c : s) {
    if (c == '\'') {
      out += "'\"'\"'";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

bool run_capture(const std::string& cmd, std::string& out_text) {
  FILE* fp = popen(cmd.c_str(), "r");
  if (!fp) return false;
  out_text.clear();
  char buf[4096];
  while (fgets(buf, sizeof(buf), fp)) {
    out_text += buf;
  }
  int rc = pclose(fp);
  if (rc != 0) return false;
  return !out_text.empty();
}

}  // namespace

std::string Preprocessor::preprocess_file(const std::string& path) const {
  const std::string qpath = shell_quote(path);

  // Prefer reference compiler preprocessor when available.
  // We intentionally avoid hard failure if ccc is missing so this remains usable
  // in environments without Rust toolchain/binary.
  std::vector<std::string> commands = {
      "ccc -E -P -x c " + qpath + " 2>/dev/null",
      "cc -E -P -x c " + qpath + " 2>/dev/null",
      "cpp -E -P " + qpath + " 2>/dev/null",
  };

  std::string result;
  for (const std::string& cmd : commands) {
    if (run_capture(cmd, result)) {
      return result;
    }
  }
  return read_file(path);
}

}  // namespace tinyc2ll::frontend_cxx
