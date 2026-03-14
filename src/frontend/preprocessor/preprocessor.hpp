#ifndef TINYC2LL_FRONTEND_CXX_PREPROCESSOR_HPP
#define TINYC2LL_FRONTEND_CXX_PREPROCESSOR_HPP

#include "pp_macro_def.hpp"

#include <set>
#include <string>
#include <unordered_set>
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

  // Main entry points.
  std::string preprocess_file(const std::string& path);
  std::string preprocess_source(const std::string& source,
                                const std::string& filename = "<stdin>");

  // Include path configuration (GCC-compatible buckets).
  // Search order for #include "file.h": current dir → quote → normal → system → after
  // Search order for #include <file.h>: normal → system → after
  void add_quote_include_path(const std::string& path);   // -iquote
  void add_include_path(const std::string& path);         // -I
  void add_system_include_path(const std::string& path);  // -isystem
  void add_after_include_path(const std::string& path);   // -idirafter

  // Define/undefine macros from driver (for -D/-U command-line flags).
  // define_macro("FOO") defines FOO as 1.
  // define_macro("FOO=bar") defines FOO as bar.
  void define_macro(const std::string& def);
  void undefine_macro(const std::string& name);

  // Side-channel diagnostics (framework hooks).
  const std::vector<PreprocessorDiagnostic>& errors() const { return errors_; }
  const std::vector<PreprocessorDiagnostic>& warnings() const { return warnings_; }

private:
  struct ConditionalFrame {
    bool parent_active = true;
    bool any_taken = false;
    bool this_active = true;
    bool saw_else = false;
  };

  // Top-level pipeline.
  std::string preprocess_text(const std::string& source, const std::string& file,
                              int include_depth);

  // Directive and expansion helpers.
  void process_directive(const std::string& raw_line, std::string& out,
                         const std::string& current_file, int line_no,
                         int include_depth);
  std::string expand_line(const std::string& line);
  std::string expand_text(const std::string& text,
                          std::unordered_set<std::string> disabled);
  std::string expand_funclike_call(const MacroDef& def,
                                   const std::vector<std::string>& raw_args,
                                   std::unordered_set<std::string> disabled);

  void handle_define(const std::string& args, const std::string& file, int line_no);
  void handle_undef(const std::string& args);
  void handle_ifdef(const std::string& args, bool is_ifndef);
  void handle_if(const std::string& args);
  void handle_elif(const std::string& args);
  void handle_else();
  void handle_endif();

  bool evaluate_if_expr(const std::string& expr);
  bool is_active() const;

  // Include resolution.
  std::string handle_include(const std::string& args, const std::string& current_file,
                             int include_depth, int line_no);
  bool can_resolve_include(const std::string& path_arg, const std::string& current_file);

private:
  MacroTable macros_;
  std::vector<ConditionalFrame> cond_stack_;

  // Include search path buckets (GCC-compatible order).
  std::vector<std::string> quote_include_paths_;   // -iquote: quoted includes only
  std::vector<std::string> normal_include_paths_;   // -I: both quoted and angle
  std::vector<std::string> system_include_paths_;   // -isystem: both quoted and angle
  std::vector<std::string> after_include_paths_;    // -idirafter: searched last

  std::vector<PreprocessorDiagnostic> errors_;
  std::vector<PreprocessorDiagnostic> warnings_;

  bool needs_external_fallback_ = false;
  std::string base_file_;

  // Virtual source location (updated by #line directives).
  // virtual_line at physical line L = L + virtual_line_offset_.
  int virtual_line_offset_ = 0;
  std::string virtual_file_;

  // __COUNTER__ state — monotonically increasing across all files.
  int counter_ = 0;

  // #pragma once — set of canonical file paths that should not be re-included.
  std::set<std::string> pragma_once_files_;
};

}  // namespace tinyc2ll::frontend_cxx

#endif
