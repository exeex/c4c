#include "pp_include.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace c4c {

std::string dirname_of(const std::string& path) {
  size_t pos = path.find_last_of("/");
  if (pos == std::string::npos) return std::string();
  if (pos == 0) return "/";
  return path.substr(0, pos);
}

bool read_file(const std::string& path, std::string* out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream buf;
  buf << in.rdbuf();
  *out = buf.str();
  return true;
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

std::string preprocess_external(const std::string& path) {
  const std::string qpath = shell_quote(path);
  std::vector<std::string> commands = {
      "ccc -E -P -x c " + qpath + " 2>/dev/null",
      "cc -E -P -x c " + qpath + " 2>/dev/null",
      "cpp -E -P " + qpath + " 2>/dev/null",
  };

  std::string result;
  for (const std::string& cmd : commands) {
    if (run_capture(cmd, result)) return result;
  }
  return std::string();
}

}  // namespace c4c
