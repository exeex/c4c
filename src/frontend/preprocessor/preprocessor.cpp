#include "preprocessor.hpp"
#include "pp_cond.hpp"
#include "pp_include.hpp"
#include "pp_macro_expand.hpp"
#include "pp_pragma.hpp"
#include "pp_predefined.hpp"
#include "pp_text.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace tinyc2ll::frontend_cxx {

Preprocessor::Preprocessor() {
  init_predefined_macros(macros_);
}

std::string Preprocessor::preprocess_file(const std::string& path) {
  std::string source;
  if (!read_file(path, &source)) {
    throw std::runtime_error("failed to open file: " + path);
  }

  errors_.clear();
  warnings_.clear();
  cond_stack_.clear();
  needs_external_fallback_ = false;
  base_file_ = path;
  counter_ = 0;

  // Set __BASE_FILE__ — remains constant across all included files.
  macros_["__BASE_FILE__"] = MacroDef{"__BASE_FILE__", false, false, {},
                                       "\"" + path + "\""};

  // Emit initial line marker (GCC-compatible: # 1 "filename").
  std::string internal;
  internal += "# 1 \"" + path + "\"\n";
  internal += preprocess_text(source, path, 0);
  return internal;
}

std::string Preprocessor::preprocess_source(const std::string& source,
                                            const std::string& filename) {
  errors_.clear();
  warnings_.clear();
  cond_stack_.clear();
  needs_external_fallback_ = false;
  base_file_ = filename;
  counter_ = 0;

  macros_["__BASE_FILE__"] = MacroDef{"__BASE_FILE__", false, false, {},
                                       "\"" + filename + "\""};

  std::string internal;
  internal += "# 1 \"" + filename + "\"\n";
  internal += preprocess_text(source, filename, 0);
  return internal;
}

std::string Preprocessor::preprocess_text(const std::string& source,
                                          const std::string& file,
                                          int include_depth) {
  if (include_depth > 200) {
    errors_.push_back(PreprocessorDiagnostic{file, 1, 1,
                                             "include depth exceeds limit (200)"});
    return "\n";
  }

  // Save and reset virtual source location for this translation unit / included file.
  int saved_voffset = virtual_line_offset_;
  std::string saved_vfile = virtual_file_;
  virtual_line_offset_ = 0;
  virtual_file_ = file;

  std::string p2 = join_continued_lines(source);
  std::string p3 = strip_comments(p2);

  std::string out;
  std::istringstream in(p3);
  std::string line;
  int line_no = 0;

  while (std::getline(in, line)) {
    ++line_no;

    // Keep __LINE__ and __FILE__ macros in sync with current virtual source position.
    // These are updated before processing the line so that both directive expressions
    // (e.g. #if __LINE__ == 1000) and non-directive text see the correct value.
    int vline = line_no + virtual_line_offset_;
    macros_["__LINE__"] = MacroDef{"__LINE__", false, false, {}, std::to_string(vline)};
    macros_["__FILE__"] = MacroDef{"__FILE__", false, false, {},
                                   "\"" + virtual_file_ + "\""};

    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    bool is_directive = (i < line.size() && line[i] == '#');

    if (is_directive) {
      process_directive(line.substr(i + 1), out, file, line_no, include_depth);
      continue;
    }

    if (is_active()) {
      out += expand_line(line);
      out.push_back('\n');
    } else {
      // Preserve line count in inactive conditional branches.
      out.push_back('\n');
    }
  }

  // Restore caller's virtual source location (important for include return).
  virtual_line_offset_ = saved_voffset;
  virtual_file_ = saved_vfile;

  return out;
}

void Preprocessor::process_directive(const std::string& raw_line, std::string& out,
                                     const std::string& current_file, int line_no,
                                     int include_depth) {
  auto [key, rest] = split_directive(raw_line);

  if (key.empty()) {
    out.push_back('\n');
    return;
  }

  if (key == "if") {
    handle_if(rest);
    out.push_back('\n');
    return;
  }
  if (key == "ifdef") {
    handle_ifdef(rest, false);
    out.push_back('\n');
    return;
  }
  if (key == "ifndef") {
    handle_ifdef(rest, true);
    out.push_back('\n');
    return;
  }
  if (key == "elif") {
    handle_elif(rest);
    out.push_back('\n');
    return;
  }
  if (key == "else") {
    handle_else();
    out.push_back('\n');
    return;
  }
  if (key == "endif") {
    handle_endif();
    out.push_back('\n');
    return;
  }

  if (!is_active()) {
    // In inactive branches only conditional directives affect state.
    out.push_back('\n');
    return;
  }

  if (key == "define") {
    handle_define(rest, current_file, line_no);
  } else if (key == "undef") {
    handle_undef(rest);
  } else if (key == "include") {
    out += handle_include(rest, current_file, include_depth, line_no);
    return;
  } else if (key == "error") {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             trim_copy(rest)});
  } else if (key == "warning") {
    warnings_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                               trim_copy(rest)});
  } else if (key == "pragma") {
    PragmaResult pr = dispatch_pragma(rest, current_file, line_no);
    if (pr == PragmaResult::Unhandled) {
      needs_external_fallback_ = true;
    }
  } else if (key == "line") {
    // C11 6.10.4: #line <digit-sequence> ["filename"]
    // Macro-expand the argument first (e.g. #line LINE where LINE=1000).
    std::string expanded = expand_line(trim_copy(rest));
    size_t i = 0;
    while (i < expanded.size() && std::isspace(static_cast<unsigned char>(expanded[i]))) ++i;
    size_t num_start = i;
    while (i < expanded.size() && std::isdigit(static_cast<unsigned char>(expanded[i]))) ++i;
    if (i == num_start) {
      errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                               "#line directive: missing line number"});
    } else {
      int new_lineno = std::stoi(expanded.substr(num_start, i - num_start));
      while (i < expanded.size() && std::isspace(static_cast<unsigned char>(expanded[i]))) ++i;
      std::string new_file = virtual_file_;
      if (i < expanded.size() && expanded[i] == '"') {
        size_t j = i + 1;
        while (j < expanded.size() && expanded[j] != '"') ++j;
        new_file = expanded.substr(i + 1, j - i - 1);
      }
      // Update virtual location: next physical line (line_no+1) maps to new_lineno.
      virtual_line_offset_ = new_lineno - (line_no + 1);
      virtual_file_ = new_file;
      // Emit a GCC-compatible line marker so downstream lexer can track source positions.
      out += "# " + std::to_string(new_lineno) + " \"" + new_file + "\"\n";
      return;  // line marker already has the newline
    }
  } else {
    // TODO(preprocessor): unknown directives should be handled more carefully
    // (diagnostic policy, extension points).
    needs_external_fallback_ = true;
  }

  out.push_back('\n');
}

void Preprocessor::handle_define(const std::string& args,
                                 const std::string& file,
                                 int line_no) {
  std::string s = args;
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
  if (i >= s.size() || !is_ident_start(s[i])) {
    errors_.push_back(PreprocessorDiagnostic{file, line_no, 1,
                                             "invalid #define: missing macro name"});
    return;
  }

  size_t n0 = i++;
  while (i < s.size() && is_ident_continue(s[i])) ++i;
  std::string name = s.substr(n0, i - n0);

  MacroDef def;
  def.name = name;

  // Function-like macro has no whitespace between name and '('.
  if (i < s.size() && s[i] == '(') {
    def.function_like = true;
    ++i;
    std::string param;
    bool seen_variadic = false;

    while (i < s.size()) {
      char c = s[i];
      if (c == ')') {
        if (!param.empty()) {
          std::string p = trim_copy(param);
          if (p == "...") {
            def.variadic = true;
            seen_variadic = true;
          } else {
            def.params.push_back(p);
          }
          param.clear();
        }
        ++i;
        break;
      }
      if (c == ',') {
        std::string p = trim_copy(param);
        if (!p.empty()) {
          if (p == "...") {
            def.variadic = true;
            seen_variadic = true;
          } else {
            def.params.push_back(p);
          }
        }
        param.clear();
        ++i;
        continue;
      }
      param.push_back(c);
      ++i;
    }

    (void)seen_variadic;  // variadic macros are now handled natively
  }

  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
  def.body = (i < s.size()) ? s.substr(i) : std::string();

  macros_[name] = std::move(def);
}

void Preprocessor::handle_undef(const std::string& args) {
  std::string name = trim_copy(args);
  if (name.empty()) return;
  macros_.erase(name);
}

void Preprocessor::handle_ifdef(const std::string& args, bool is_ifndef) {
  std::string name = trim_copy(args);
  bool defined = macros_.find(name) != macros_.end();
  bool take = is_ifndef ? !defined : defined;

  ConditionalFrame f;
  f.parent_active = is_active();
  f.this_active = f.parent_active && take;
  f.any_taken = f.this_active;
  cond_stack_.push_back(f);
}

void Preprocessor::handle_if(const std::string& args) {
  bool take = evaluate_if_expr(args);

  ConditionalFrame f;
  f.parent_active = is_active();
  f.this_active = f.parent_active && take;
  f.any_taken = f.this_active;
  cond_stack_.push_back(f);
}

void Preprocessor::handle_elif(const std::string& args) {
  if (cond_stack_.empty()) {
    needs_external_fallback_ = true;
    return;
  }

  ConditionalFrame& f = cond_stack_.back();
  if (f.saw_else) {
    needs_external_fallback_ = true;
    return;
  }

  bool take = false;
  if (!f.any_taken) {
    take = evaluate_if_expr(args);
  }
  f.this_active = f.parent_active && !f.any_taken && take;
  if (f.this_active) f.any_taken = true;
}

void Preprocessor::handle_else() {
  if (cond_stack_.empty()) {
    needs_external_fallback_ = true;
    return;
  }

  ConditionalFrame& f = cond_stack_.back();
  if (f.saw_else) {
    needs_external_fallback_ = true;
    return;
  }
  f.saw_else = true;

  f.this_active = f.parent_active && !f.any_taken;
  if (f.this_active) f.any_taken = true;
}

void Preprocessor::handle_endif() {
  if (cond_stack_.empty()) {
    needs_external_fallback_ = true;
    return;
  }
  cond_stack_.pop_back();
}

bool Preprocessor::evaluate_if_expr(const std::string& expr) {
  auto is_defined_name = [this](const std::string& name) {
    return macros_.find(name) != macros_.end();
  };

  // C11-ish pipeline:
  // 1) resolve defined()/__has_* intrinsics
  // 2) macro expansion
  // 3) resolve again in case expansion introduced intrinsics
  std::string r1 = resolve_defined_and_intrinsics(expr, is_defined_name);
  std::string expanded = expand_line(r1);
  std::string r2 = resolve_defined_and_intrinsics(expanded, is_defined_name);
  std::string s = trim_copy(r2);
  if (s.empty()) return false;

  bool ok = true;
  IfExprParser parser(s, is_defined_name);
  ExprValue v = parser.parse(&ok);
  if (!ok) {
    // Keep behavior compatible while parser coverage expands.
    needs_external_fallback_ = true;
    return false;
  }
  return as_bool(v);
}

bool Preprocessor::is_active() const {
  if (cond_stack_.empty()) return true;
  return cond_stack_.back().this_active;
}

// Single-pass recursive expansion with hideset support (C11 6.10.3.4 "blue paint").
// 'disabled' is the set of macro names that must NOT be expanded in this context.
// Each call handles one left-to-right pass; recursive calls handle nested expansions
// and rescan with the macro's own name added to the disabled set.
std::string Preprocessor::expand_text(const std::string& text,
                                      std::unordered_set<std::string> disabled) {
  std::string out;
  out.reserve(text.size() + 16);

  bool in_str = false;
  bool in_chr = false;

  for (size_t i = 0; i < text.size();) {
    char c = text[i];

    if (!in_str && !in_chr && is_ident_start(c)) {
      size_t j = i + 1;
      while (j < text.size() && is_ident_continue(text[j])) ++j;
      std::string ident = text.substr(i, j - i);

      auto it = macros_.find(ident);
      if (it != macros_.end() && disabled.find(ident) == disabled.end()) {
        const MacroDef& def = it->second;
        if (def.function_like) {
          // Function-like: only expand if immediately followed by '('.
          size_t k = j;
          while (k < text.size() && (text[k] == ' ' || text[k] == '\t')) ++k;
          if (k < text.size() && text[k] == '(') {
            ++k;  // skip '('
            std::vector<std::string> raw_args = collect_funclike_args(text, &k);
            // Normalise: F() with no params → empty args list.
            if (def.params.empty() && !def.variadic &&
                raw_args.size() == 1 && trim_copy(raw_args[0]).empty()) {
              raw_args.clear();
            }
            out += expand_funclike_call(def, raw_args, disabled);
            i = k;
          } else {
            // No '(' — not a macro invocation; leave as-is.
            out += ident;
            i = j;
          }
        } else {
          // Object-like: expand body with disabled ∪ {ident}.
          // Exception: L/u/U/u8 immediately followed by ' or " form a wide/unicode
          // character/string literal prefix — do NOT expand them as macros.
          if ((ident == "L" || ident == "u" || ident == "U" || ident == "u8") &&
              j < text.size() && (text[j] == '\'' || text[j] == '"')) {
            out += ident;
            i = j;
          } else if (ident == "__COUNTER__") {
            // __COUNTER__ increments each time it is expanded.
            out += std::to_string(counter_++);
            i = j;
          } else {
            std::unordered_set<std::string> new_disabled = disabled;
            new_disabled.insert(ident);
            out += expand_text(def.body, std::move(new_disabled));
            i = j;
          }
        }
      } else {
        out += ident;
        i = j;
      }
      continue;
    }

    out.push_back(c);

    if (!in_chr && c == '"' && (i == 0 || text[i - 1] != '\\')) {
      in_str = !in_str;
    } else if (!in_str && c == '\'' && (i == 0 || text[i - 1] != '\\')) {
      in_chr = !in_chr;
    }

    ++i;
  }

  return out;
}

std::string Preprocessor::expand_funclike_call(const MacroDef& def,
                                               const std::vector<std::string>& raw_args,
                                               std::unordered_set<std::string> disabled) {
  // Prescan-expand each argument (C11 6.10.3.1 step 2).
  // Arguments use the caller's disabled set (not yet inside the macro body).
  std::vector<std::string> exp_args;
  exp_args.reserve(raw_args.size());
  for (const auto& a : raw_args) {
    exp_args.push_back(expand_text(trim_copy(a), disabled));
  }

  // Build variadic join (everything beyond named params).
  std::string va_raw, va_exp;
  if (def.variadic) {
    for (size_t k = def.params.size(); k < raw_args.size(); ++k) {
      if (k > def.params.size()) { va_raw += ","; va_exp += ","; }
      va_raw += raw_args[k];
      if (k < raw_args.size()) va_exp += expand_text(trim_copy(raw_args[k]), disabled);
    }
    // GNU extension: ", ## __VA_ARGS__" removal handled in substitute_funclike_body.
  }

  // Substitute body: handles #, ## and parameter replacement.
  std::string substituted = substitute_funclike_body(def.body, def.params,
                                                     raw_args, exp_args,
                                                     def.variadic, va_raw, va_exp);

  // Rescan result for further macro expansion (C11 6.10.3.4).
  // Add the macro's own name to the disabled set to prevent self-re-expansion.
  disabled.insert(def.name);
  return expand_text(substituted, std::move(disabled));
}

std::string Preprocessor::expand_line(const std::string& line) {
  return expand_text(line, {});
}

void Preprocessor::define_macro(const std::string& def) {
  auto eq = def.find('=');
  std::string name, body;
  if (eq == std::string::npos) {
    name = def;
    body = "1";
  } else {
    name = def.substr(0, eq);
    body = def.substr(eq + 1);
  }
  macros_[name] = MacroDef{name, false, false, {}, body};
}

void Preprocessor::undefine_macro(const std::string& name) {
  macros_.erase(name);
}

void Preprocessor::add_quote_include_path(const std::string& path) {
  quote_include_paths_.push_back(path);
}

void Preprocessor::add_include_path(const std::string& path) {
  normal_include_paths_.push_back(path);
}

void Preprocessor::add_system_include_path(const std::string& path) {
  system_include_paths_.push_back(path);
}

void Preprocessor::add_after_include_path(const std::string& path) {
  after_include_paths_.push_back(path);
}

std::string Preprocessor::handle_include(const std::string& args,
                                         const std::string& current_file,
                                         int include_depth,
                                         int line_no) {
  std::string s = trim_copy(args);
  if (s.empty()) {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             "empty #include"});
    return "\n";
  }

  bool is_quoted = s.size() >= 2 && s.front() == '"' && s.back() == '"';
  bool is_angle  = s.size() >= 2 && s.front() == '<' && s.back() == '>';

  if (!is_quoted && !is_angle) {
    // Computed include — not yet supported.
    needs_external_fallback_ = true;
    return "\n";
  }

  std::string rel = s.substr(1, s.size() - 2);

  // Try to resolve the include file using GCC-compatible search order.
  // #include "file.h": current dir → quote → normal → system → after
  // #include <file.h>: normal → system → after
  auto try_resolve = [&](const std::string& dir) -> std::string {
    std::string full = dir.empty() ? rel : (dir + "/" + rel);
    std::string content;
    if (read_file(full, &content)) return full;
    return {};
  };

  std::string resolved;

  if (is_quoted) {
    // 1. Current file's directory (always first for quoted includes)
    std::string base_dir = dirname_of(current_file);
    resolved = try_resolve(base_dir);

    // 2. Quote include paths (-iquote)
    if (resolved.empty()) {
      for (const auto& dir : quote_include_paths_) {
        resolved = try_resolve(dir);
        if (!resolved.empty()) break;
      }
    }
  }

  // 3. Normal include paths (-I) — both quoted and angle
  if (resolved.empty()) {
    for (const auto& dir : normal_include_paths_) {
      resolved = try_resolve(dir);
      if (!resolved.empty()) break;
    }
  }

  // 4. System include paths (-isystem)
  if (resolved.empty()) {
    for (const auto& dir : system_include_paths_) {
      resolved = try_resolve(dir);
      if (!resolved.empty()) break;
    }
  }

  // 5. After include paths (-idirafter)
  if (resolved.empty()) {
    for (const auto& dir : after_include_paths_) {
      resolved = try_resolve(dir);
      if (!resolved.empty()) break;
    }
  }

  if (resolved.empty()) {
    if (is_angle) {
      // Angle includes without configured paths — fall back to external.
      needs_external_fallback_ = true;
    } else {
      errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                               "include file not found: " + rel});
    }
    return "\n";
  }

  std::string child_src;
  if (!read_file(resolved, &child_src)) {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             "include file not found: " + rel});
    return "\n";
  }

  // Emit GCC-compatible include enter/return markers.
  // Flag 1 = entering a new file, flag 2 = returning to a file.
  std::string result;
  result += "# 1 \"" + resolved + "\" 1\n";
  result += preprocess_text(child_src, resolved, include_depth + 1);
  result += "# " + std::to_string(line_no + 1) + " \"" + current_file + "\" 2\n";
  return result;
}

}  // namespace tinyc2ll::frontend_cxx
