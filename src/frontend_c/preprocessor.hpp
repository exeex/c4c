#ifndef TINYC2LL_FRONTEND_CXX_PREPROCESSOR_HPP
#define TINYC2LL_FRONTEND_CXX_PREPROCESSOR_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace tinyc2ll::frontend_cxx {

struct PreprocessorDiagnostic {
  std::string file;
  int line = 0;
  int column = 0;
  std::string message;
};

class Preprocessor {
public:
  Preprocessor();

  // Main entry point used by the frontend driver.
  std::string preprocess_file(const std::string& path);

  // Side-channel diagnostics (framework hooks).
  const std::vector<PreprocessorDiagnostic>& errors() const { return errors_; }
  const std::vector<PreprocessorDiagnostic>& warnings() const { return warnings_; }

private:
  struct MacroDef {
    std::string name;
    bool function_like = false;
    bool variadic = false;
    std::vector<std::string> params;
    std::string body;
  };

  struct ConditionalFrame {
    bool parent_active = true;
    bool any_taken = false;
    bool this_active = true;
    bool saw_else = false;
  };

  // Top-level pipeline.
  std::string preprocess_text(const std::string& source, const std::string& file,
                              int include_depth);

  // Translation phase helpers.
  static std::string join_continued_lines(const std::string& source);
  static std::string strip_comments(const std::string& source);

  // Directive and expansion helpers.
  void process_directive(const std::string& raw_line, std::string& out,
                         const std::string& current_file, int line_no,
                         int include_depth);
  std::string expand_line(const std::string& line);
  std::string expand_object_like_once(const std::string& line, bool* changed);
  std::string expand_funclike_call(const MacroDef& def,
                                   const std::vector<std::string>& raw_args);

  void handle_define(const std::string& args, const std::string& file, int line_no);
  void handle_undef(const std::string& args);
  void handle_ifdef(const std::string& args, bool is_ifndef);
  void handle_if(const std::string& args);
  void handle_elif(const std::string& args);
  void handle_else();
  void handle_endif();

  bool evaluate_if_expr(const std::string& expr);
  bool is_active() const;

  // Include resolution framework.
  std::string handle_include(const std::string& args, const std::string& current_file,
                             int include_depth, int line_no);
  static std::string dirname_of(const std::string& path);
  static bool read_file(const std::string& path, std::string* out);

  // External fallback to preserve current behavior while framework grows.
  std::string preprocess_external(const std::string& path) const;
  static std::string shell_quote(const std::string& s);
  static bool run_capture(const std::string& cmd, std::string& out_text);

private:
  std::unordered_map<std::string, MacroDef> macros_;
  std::vector<ConditionalFrame> cond_stack_;
  std::vector<std::string> include_paths_;
  std::vector<PreprocessorDiagnostic> errors_;
  std::vector<PreprocessorDiagnostic> warnings_;

  bool needs_external_fallback_ = false;
  std::string base_file_;

  // Virtual source location (updated by #line directives).
  // virtual_line at physical line L = L + virtual_line_offset_.
  int virtual_line_offset_ = 0;
  std::string virtual_file_;
};

}  // namespace tinyc2ll::frontend_cxx

#endif
