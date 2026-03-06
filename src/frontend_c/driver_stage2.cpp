#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {

int run_cmd(const std::vector<std::string>& args) {
  if (args.empty()) return 127;
  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (const std::string& s : args) argv.push_back(const_cast<char*>(s.c_str()));
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid < 0) {
    std::perror("fork");
    return 127;
  }
  if (pid == 0) {
    execvp(argv[0], argv.data());
    std::perror("execvp");
    _exit(127);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    std::perror("waitpid");
    return 127;
  }
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return 127;
}

bool starts_with(std::string_view s, std::string_view p) {
  return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

bool is_c_source(const std::string& s) {
  return s.size() >= 2 && s[0] != '-' && s.rfind(".c") == s.size() - 2;
}

bool is_dep_flag(const std::string& a) {
  return a == "-M" || a == "-MM" || a == "-MD" || a == "-MMD" || a == "-MG" ||
         a == "-MP" || a == "-MT" || a == "-MQ" || a == "-MF";
}

bool needs_value(const std::string& a) {
  return a == "-o" || a == "-x" || a == "-MF" || a == "-MT" || a == "-MQ" ||
         a == "-isystem" || a == "-idirafter" || a == "-iquote" || a == "-include" ||
         a == "-imacros" || a == "-target";
}

std::string detect_stage1_path(const char* argv0) {
  if (const char* env = std::getenv("TINYC2LL_STAGE1")) return env;
  fs::path p = fs::absolute(argv0);
  fs::path cand = p.parent_path() / "tiny-c2ll-stage1";
  return cand.string();
}

std::string detect_clang_path() {
  if (const char* env = std::getenv("TINYC2LL_CLANG")) return env;
  return "clang";
}

std::string make_temp_file(const char* suffix) {
  std::string path = "/tmp/tinyc2ll_stage2_XXXXXX";
  path += suffix;
  int fd = mkstemps(path.data(), static_cast<int>(std::strlen(suffix)));
  if (fd < 0) return {};
  close(fd);
  return path;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> args;
  args.reserve(argc > 1 ? argc - 1 : 0);
  for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

  const std::string clang_bin = detect_clang_path();
  const std::string stage1_bin = detect_stage1_path(argv[0]);

  // Important for Linux Kbuild detection: first line must contain "clang".
  if (args.size() == 1 && args[0] == "--version") {
    return run_cmd({clang_bin, "--version"});
  }

  bool compile_only = false;
  bool seen_E = false;
  bool seen_S = false;
  std::string output;
  std::vector<std::string> sources;

  for (size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    if (a == "-c") compile_only = true;
    if (a == "-E") seen_E = true;
    if (a == "-S") seen_S = true;
    if (a == "-o" && i + 1 < args.size()) {
      output = args[i + 1];
      ++i;
      continue;
    }
    if (starts_with(a, "-o") && a.size() > 2) output = a.substr(2);
    if (is_c_source(a)) sources.push_back(a);
    if (needs_value(a) && i + 1 < args.size()) ++i;
  }

  const bool stage2_mode =
      compile_only && !seen_E && !seen_S && sources.size() == 1 && !output.empty();

  if (!stage2_mode) {
    std::vector<std::string> passthrough;
    passthrough.reserve(args.size() + 1);
    passthrough.push_back(clang_bin);
    passthrough.insert(passthrough.end(), args.begin(), args.end());
    return run_cmd(passthrough);
  }

  const std::string src = sources[0];
  const std::string tmp_i = make_temp_file(".i");
  const std::string tmp_ll = make_temp_file(".ll");
  if (tmp_i.empty() || tmp_ll.empty()) {
    std::cerr << "error: failed to create temp files\n";
    return 1;
  }

  std::vector<std::string> pp;
  pp.push_back(clang_bin);
  pp.push_back("-E");
  pp.push_back("-P");
  pp.push_back("-x");
  pp.push_back("c");

  for (size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    if (a == "-c" || a == "-S" || a == "-E") continue;
    if (a == "-o") {
      if (i + 1 < args.size()) ++i;
      continue;
    }
    if (starts_with(a, "-o")) continue;
    if (is_dep_flag(a)) {
      if (needs_value(a) && i + 1 < args.size()) ++i;
      continue;
    }
    pp.push_back(a);
    if (needs_value(a) && i + 1 < args.size()) pp.push_back(args[++i]);
  }
  pp.push_back("-o");
  pp.push_back(tmp_i);

  int rc = run_cmd(pp);
  if (rc != 0) {
    fs::remove(tmp_i);
    fs::remove(tmp_ll);
    return rc;
  }

  rc = run_cmd({stage1_bin, tmp_i, "-o", tmp_ll});
  if (rc != 0) {
    fs::remove(tmp_i);
    fs::remove(tmp_ll);
    return rc;
  }

  std::vector<std::string> backend;
  backend.push_back(clang_bin);
  backend.push_back("-Wno-unused-command-line-argument");
  backend.push_back("-c");
  backend.push_back("-x");
  backend.push_back("ir");
  backend.push_back(tmp_ll);
  backend.push_back("-o");
  backend.push_back(output);

  for (size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    if (a == "-target" && i + 1 < args.size()) {
      backend.push_back(a);
      backend.push_back(args[++i]);
      continue;
    }
    if (starts_with(a, "--target=") || starts_with(a, "-target=") ||
        starts_with(a, "-m") || starts_with(a, "-f") ||
        starts_with(a, "-O") || starts_with(a, "-g")) {
      backend.push_back(a);
    }
  }

  rc = run_cmd(backend);
  fs::remove(tmp_i);
  fs::remove(tmp_ll);
  return rc;
}
