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

namespace {

bool parse_single_identifier(const std::string& text, std::string* ident) {
  size_t i = 0;
  while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
  if (i >= text.size() || !is_ident_start(text[i])) return false;

  size_t j = i + 1;
  while (j < text.size() && is_ident_continue(text[j])) ++j;
  while (j < text.size() && std::isspace(static_cast<unsigned char>(text[j]))) ++j;
  if (j != text.size()) return false;

  *ident = text.substr(i, j - i);
  ident->erase(ident->find_last_not_of(" \t\r\n") + 1);
  return true;
}

}  // namespace

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
  pragma_once_files_.clear();
  include_guard_map_.clear();
  include_resolve_cache_.clear();
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
  pragma_once_files_.clear();
  include_guard_map_.clear();
  include_resolve_cache_.clear();
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

namespace {
// Check if a line has more '(' than ')' outside of string/char literals,
// indicating a multi-line function-like macro invocation.
bool has_unbalanced_parens(const std::string& line) {
  int depth = 0;
  bool in_str = false, in_chr = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (in_str) {
      if (c == '\\' && i + 1 < line.size()) { ++i; continue; }
      if (c == '"') in_str = false;
      continue;
    }
    if (in_chr) {
      if (c == '\\' && i + 1 < line.size()) { ++i; continue; }
      if (c == '\'') in_chr = false;
      continue;
    }
    if (c == '"') { in_str = true; continue; }
    if (c == '\'') { in_chr = true; continue; }
    if (c == '(') ++depth;
    else if (c == ')') --depth;
  }
  return depth > 0;
}
}  // namespace

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
      // Multi-line function-like macro invocation: if the line has unbalanced
      // parentheses (more '(' than ')'), accumulate subsequent lines until
      // balanced so that expand_line sees the complete argument list.
      // Interleaved directives (#define, #undef, etc.) are processed in-place.
      int extra_lines = 0;
      while (has_unbalanced_parens(line)) {
        std::string next;
        if (!std::getline(in, next)) break;
        ++line_no;
        ++extra_lines;
        // Update __LINE__ for any directives encountered mid-accumulation.
        int vl = line_no + virtual_line_offset_;
        macros_["__LINE__"] = MacroDef{"__LINE__", false, false, {}, std::to_string(vl)};
        // Check if the next line is a directive.
        size_t ni = 0;
        while (ni < next.size() && (next[ni] == ' ' || next[ni] == '\t')) ++ni;
        if (ni < next.size() && next[ni] == '#') {
          // Process directive in-place (e.g., #define, #undef between args).
          process_directive(next.substr(ni + 1), out, file, line_no, include_depth);
        } else {
          // Expand the line with current macro state before accumulating,
          // so that directives processed in-between take correct effect.
          line.push_back(' ');
          line += expand_line(next);
        }
      }
      out += expand_line(line);
      out.push_back('\n');
      // Emit blank lines to preserve line count for accumulated lines.
      for (int el = 0; el < extra_lines; ++el) {
        out.push_back('\n');
      }
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
    out += handle_include(rest, current_file, include_depth, line_no, false);
    return;
  } else if (key == "include_next") {
    out += handle_include(rest, current_file, include_depth, line_no, true);
    return;
  } else if (key == "error") {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             trim_copy(rest)});
  } else if (key == "warning") {
    warnings_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                               trim_copy(rest)});
  } else if (key == "pragma") {
    std::string pragma_text = trim_copy(rest);
    if (pragma_text == "once") {
      pragma_once_files_.insert(current_file);
    } else if (pragma_text.substr(0, 10) == "push_macro") {
      // #pragma push_macro("NAME") — save current macro definition.
      auto q1 = pragma_text.find('"');
      auto q2 = pragma_text.find('"', q1 + 1);
      if (q1 != std::string::npos && q2 != std::string::npos) {
        std::string name = pragma_text.substr(q1 + 1, q2 - q1 - 1);
        auto it = macros_.find(name);
        if (it != macros_.end()) {
          macro_stack_[name].push_back(it->second);
        } else {
          // Push a sentinel to indicate the macro was undefined.
          macro_stack_[name].push_back(MacroDef{name, false, false, {}, ""});
          macro_stack_[name].back().name = "";  // sentinel: empty name = undefined
        }
      }
    } else if (pragma_text.substr(0, 9) == "pop_macro") {
      // #pragma pop_macro("NAME") — restore saved macro definition.
      auto q1 = pragma_text.find('"');
      auto q2 = pragma_text.find('"', q1 + 1);
      if (q1 != std::string::npos && q2 != std::string::npos) {
        std::string name = pragma_text.substr(q1 + 1, q2 - q1 - 1);
        auto it = macro_stack_.find(name);
        if (it != macro_stack_.end() && !it->second.empty()) {
          MacroDef saved = std::move(it->second.back());
          it->second.pop_back();
          if (saved.name.empty()) {
            // Sentinel: macro was undefined before push.
            macros_.erase(name);
          } else {
            macros_[name] = std::move(saved);
          }
        }
      }
    } else {
      PragmaResult pr = dispatch_pragma(rest, current_file, line_no);
      if (pr == PragmaResult::Unhandled) {
        needs_external_fallback_ = true;
      }
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
          } else if (p.size() > 3 && p.substr(p.size() - 3) == "...") {
            // GNU named variadic: args...
            def.variadic = true;
            seen_variadic = true;
            def.va_name = trim_copy(p.substr(0, p.size() - 3));
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
          } else if (p.size() > 3 && p.substr(p.size() - 3) == "...") {
            def.variadic = true;
            seen_variadic = true;
            def.va_name = trim_copy(p.substr(0, p.size() - 3));
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

  auto has_include_cb = [this](const std::string& arg) -> bool {
    return can_resolve_include(arg, virtual_file_);
  };

  // C11-ish pipeline:
  // 1) resolve defined()/__has_* intrinsics
  // 2) macro expansion
  // 3) resolve again in case expansion introduced intrinsics
  std::string r1 = resolve_defined_and_intrinsics(expr, is_defined_name, has_include_cb);
  std::string expanded = expand_line(r1);
  std::string r2 = resolve_defined_and_intrinsics(expanded, is_defined_name, has_include_cb);
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

    // Skip pp-numbers: digit (or . followed by digit) starts a pp-number.
    // A pp-number consists of digits, letters, underscores, dots,
    // and [eEpP][+-] sequences.  The trailing letters (e.g. F, L, ULL)
    // are part of the number and must NOT be treated as identifiers.
    if (!in_str && !in_chr &&
        (std::isdigit(static_cast<unsigned char>(c)) ||
         (c == '.' && i + 1 < text.size() &&
          std::isdigit(static_cast<unsigned char>(text[i + 1]))))) {
      size_t j = i;
      while (j < text.size()) {
        char ch = text[j];
        if (std::isdigit(static_cast<unsigned char>(ch)) ||
            std::isalpha(static_cast<unsigned char>(ch)) ||
            ch == '_' || ch == '.') {
          // [eEpP] may be followed by [+-] as part of the exponent.
          if ((ch == 'e' || ch == 'E' || ch == 'p' || ch == 'P') &&
              j + 1 < text.size() && (text[j + 1] == '+' || text[j + 1] == '-')) {
            out.push_back(ch);
            ++j;
            out.push_back(text[j]);
            ++j;
            continue;
          }
          out.push_back(ch);
          ++j;
        } else {
          break;
        }
      }
      i = j;
      continue;
    }

    if (!in_str && !in_chr && is_ident_start(c)) {
      size_t j = i + 1;
      while (j < text.size() && is_ident_continue(text[j])) ++j;
      std::string ident = text.substr(i, j - i);

      // _Pragma("...") operator (C99 6.10.9): destringize and process as #pragma.
      if (ident == "_Pragma") {
        size_t k = j;
        while (k < text.size() && (text[k] == ' ' || text[k] == '\t')) ++k;
        if (k < text.size() && text[k] == '(') {
          ++k;
          while (k < text.size() && (text[k] == ' ' || text[k] == '\t')) ++k;
          if (k < text.size() && text[k] == '"') {
            // Collect string literal
            std::string str_content;
            ++k;  // skip opening "
            while (k < text.size() && text[k] != '"') {
              if (text[k] == '\\' && k + 1 < text.size()) {
                char esc = text[k + 1];
                if (esc == '"') { str_content += '"'; k += 2; continue; }
                if (esc == '\\') { str_content += '\\'; k += 2; continue; }
              }
              str_content += text[k];
              ++k;
            }
            if (k < text.size()) ++k;  // skip closing "
            while (k < text.size() && (text[k] == ' ' || text[k] == '\t')) ++k;
            if (k < text.size() && text[k] == ')') {
              ++k;  // skip closing )
              // Process the destringized content as a pragma directive.
              process_pragma_text(str_content);
              i = k;
              continue;
            }
          }
        }
        // Not a valid _Pragma invocation — output as-is.
        out += ident;
        i = j;
        continue;
      }

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
            std::string expanded = expand_funclike_call(def, raw_args, disabled);

            // Rescan may need to combine the replacement with still-unread source
            // tokens, e.g. CAT(A,B)(x) -> AB(x) where "(x)" comes from the caller.
            while (true) {
              std::string chained_ident;
              if (!parse_single_identifier(expanded, &chained_ident)) break;

              auto chained = macros_.find(chained_ident);
              if (chained == macros_.end() || !chained->second.function_like ||
                  disabled.find(chained_ident) != disabled.end()) {
                break;
              }

              size_t p = k;
              while (p < text.size() && (text[p] == ' ' || text[p] == '\t')) ++p;
              if (p >= text.size() || text[p] != '(') break;

              ++p;  // skip '('
              std::vector<std::string> chained_args = collect_funclike_args(text, &p);
              if (chained->second.params.empty() && !chained->second.variadic &&
                  chained_args.size() == 1 && trim_copy(chained_args[0]).empty()) {
                chained_args.clear();
              }

              expanded = expand_funclike_call(chained->second, chained_args, disabled);
              k = p;
            }

            // Anti-paste: guard boundary between previous output and expansion
            if (!expanded.empty() && !out.empty()) {
              maybe_insert_token_separator(out, expanded.front());
            }
            out += expanded;
            // Anti-paste: guard boundary between expansion and next source char
            if (!out.empty() && k < text.size() &&
                !std::isspace(static_cast<unsigned char>(text[k]))) {
              maybe_insert_token_separator(out, text[k]);
            }
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
            std::string expanded_obj = expand_text(def.body, std::move(new_disabled));
            // Anti-paste: guard boundaries
            if (!expanded_obj.empty() && !out.empty()) {
              maybe_insert_token_separator(out, expanded_obj.front());
            }
            out += expanded_obj;
            if (!out.empty() && j < text.size() &&
                !std::isspace(static_cast<unsigned char>(text[j]))) {
              maybe_insert_token_separator(out, text[j]);
            }
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
      if (k > def.params.size()) { va_raw += ","; va_exp += ", "; }
      va_raw += raw_args[k];
      if (k < raw_args.size()) va_exp += expand_text(trim_copy(raw_args[k]), disabled);
    }
    // GNU extension: ", ## __VA_ARGS__" removal handled in substitute_funclike_body.
  }

  // Substitute body: handles #, ## and parameter replacement.
  std::string substituted = substitute_funclike_body(def.body, def.params,
                                                     raw_args, exp_args,
                                                     def.variadic, va_raw, va_exp,
                                                     def.va_name);

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
                                         int line_no,
                                         bool is_include_next) {
  std::string s = trim_copy(args);
  if (s.empty()) {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             "empty #include"});
    return "\n";
  }

  bool is_quoted = s.size() >= 2 && s.front() == '"' && s.back() == '"';
  bool is_angle  = s.size() >= 2 && s.front() == '<' && s.back() == '>';

  if (!is_quoted && !is_angle) {
    // Computed include: macro-expand the argument, then re-check.
    std::string expanded = trim_copy(expand_line(s));
    is_quoted = expanded.size() >= 2 && expanded.front() == '"' && expanded.back() == '"';
    is_angle  = expanded.size() >= 2 && expanded.front() == '<' && expanded.back() == '>';
    if (!is_quoted && !is_angle) {
      errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                               "computed #include did not expand to a valid path"});
      return "\n";
    }
    s = expanded;
  }

  std::string rel = s.substr(1, s.size() - 2);

  std::string resolved;

  // Include resolution cache: skip filesystem probes for previously resolved paths.
  // Cache key encodes the relative path, include style, and current directory (for quoted).
  // #include_next bypasses the cache since resolution depends on position.
  std::string cache_key;
  if (!is_include_next) {
    cache_key = rel + "|" + (is_angle ? "a" : "q:" + dirname_of(current_file));
    auto it = include_resolve_cache_.find(cache_key);
    if (it != include_resolve_cache_.end()) {
      resolved = it->second;
    }
  }

  if (resolved.empty()) {
    // Try to resolve the include file using GCC-compatible search order.
    // #include "file.h": current dir → quote → normal → system → after
    // #include <file.h>: normal → system → after
    auto try_resolve = [&](const std::string& dir) -> std::string {
      std::string full = dir.empty() ? rel : (dir + "/" + rel);
      std::string content;
      if (read_file(full, &content)) return full;
      return {};
    };

    auto starts_with = [](const std::string& path, const std::string& dir) -> bool {
      if (dir.empty()) return false;
      std::string prefix = dir;
      if (prefix.back() != '/') prefix += '/';
      return path.size() >= prefix.size() && path.compare(0, prefix.size(), prefix) == 0;
    };

    enum SearchList { SL_CurDir = 0, SL_Quote = 1, SL_Normal = 2, SL_System = 3, SL_After = 4 };
    struct SearchEntry { std::string dir; int list_id; };
    std::vector<SearchEntry> all_dirs;
    if (is_quoted) {
      all_dirs.push_back({dirname_of(current_file), SL_CurDir});
      for (const auto& d : quote_include_paths_) all_dirs.push_back({d, SL_Quote});
    }
    for (const auto& d : normal_include_paths_) all_dirs.push_back({d, SL_Normal});
    for (const auto& d : system_include_paths_) all_dirs.push_back({d, SL_System});
    for (const auto& d : after_include_paths_) all_dirs.push_back({d, SL_After});

    size_t start_idx = 0;
    if (is_include_next) {
      for (size_t i = 0; i < all_dirs.size(); ++i) {
        if (starts_with(current_file, all_dirs[i].dir) ||
            dirname_of(current_file) == all_dirs[i].dir) {
          start_idx = i + 1;
          break;
        }
      }
    }

    for (size_t i = start_idx; i < all_dirs.size() && resolved.empty(); ++i) {
      resolved = try_resolve(all_dirs[i].dir);
    }

    // Populate cache on successful resolution (not for #include_next).
    if (!resolved.empty() && !cache_key.empty()) {
      include_resolve_cache_[cache_key] = resolved;
    }
  }

  if (resolved.empty()) {
    // Try builtin header injection for well-known system headers.
    std::string builtin = get_builtin_header(rel);
    if (!builtin.empty()) {
      std::string result;
      result += "# 1 \"<builtin-" + rel + ">\" 1\n";
      result += preprocess_text(builtin, "<builtin-" + rel + ">", include_depth + 1);
      result += "# " + std::to_string(line_no + 1) + " \"" + current_file + "\" 2\n";
      return result;
    }
    if (is_angle) {
      // Angle includes without configured paths — fall back to external.
      needs_external_fallback_ = true;
    } else {
      errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                               "include file not found: " + rel});
    }
    return "\n";
  }

  // #pragma once check: skip if this file was already included with pragma once.
  if (pragma_once_files_.count(resolved)) {
    return "\n";
  }

  // Include guard optimization: skip if the file's guard macro is already defined.
  auto guard_it = include_guard_map_.find(resolved);
  if (guard_it != include_guard_map_.end()) {
    if (macros_.find(guard_it->second) != macros_.end()) {
      return "\n";
    }
  }

  std::string child_src;
  if (!read_file(resolved, &child_src)) {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             "include file not found: " + rel});
    return "\n";
  }

  // Detect include guard pattern before processing.
  if (guard_it == include_guard_map_.end()) {
    std::string guard = detect_include_guard(child_src);
    if (!guard.empty()) {
      include_guard_map_[resolved] = guard;
    }
  }

  // Emit GCC-compatible include enter/return markers.
  // Flag 1 = entering a new file, flag 2 = returning to a file.
  std::string result;
  result += "# 1 \"" + resolved + "\" 1\n";
  result += preprocess_text(child_src, resolved, include_depth + 1);
  result += "# " + std::to_string(line_no + 1) + " \"" + current_file + "\" 2\n";
  return result;
}

bool Preprocessor::can_resolve_include(const std::string& path_arg,
                                       const std::string& current_file) {
  std::string s = trim_copy(path_arg);
  if (s.empty()) return false;

  bool is_quoted = s.size() >= 2 && s.front() == '"' && s.back() == '"';
  bool is_angle  = s.size() >= 2 && s.front() == '<' && s.back() == '>';
  if (!is_quoted && !is_angle) return false;

  std::string rel = s.substr(1, s.size() - 2);

  auto try_exists = [&](const std::string& dir) -> bool {
    std::string full = dir.empty() ? rel : (dir + "/" + rel);
    std::string content;
    return read_file(full, &content);
  };

  if (is_quoted) {
    if (try_exists(dirname_of(current_file))) return true;
    for (const auto& d : quote_include_paths_)
      if (try_exists(d)) return true;
  }
  for (const auto& d : normal_include_paths_)
    if (try_exists(d)) return true;
  for (const auto& d : system_include_paths_)
    if (try_exists(d)) return true;
  for (const auto& d : after_include_paths_)
    if (try_exists(d)) return true;

  // Check builtin headers as fallback.
  if (!get_builtin_header(rel).empty()) return true;

  return false;
}

void Preprocessor::process_pragma_text(const std::string& pragma_text) {
  std::string s = trim_copy(pragma_text);
  if (s == "once") {
    pragma_once_files_.insert(virtual_file_);
  } else if (s.substr(0, 10) == "push_macro") {
    auto q1 = s.find('"');
    auto q2 = s.find('"', q1 + 1);
    if (q1 != std::string::npos && q2 != std::string::npos) {
      std::string name = s.substr(q1 + 1, q2 - q1 - 1);
      auto it = macros_.find(name);
      if (it != macros_.end()) {
        macro_stack_[name].push_back(it->second);
      } else {
        macro_stack_[name].push_back(MacroDef{name, false, false, {}, ""});
        macro_stack_[name].back().name = "";
      }
    }
  } else if (s.substr(0, 9) == "pop_macro") {
    auto q1 = s.find('"');
    auto q2 = s.find('"', q1 + 1);
    if (q1 != std::string::npos && q2 != std::string::npos) {
      std::string name = s.substr(q1 + 1, q2 - q1 - 1);
      auto it = macro_stack_.find(name);
      if (it != macro_stack_.end() && !it->second.empty()) {
        MacroDef saved = std::move(it->second.back());
        it->second.pop_back();
        if (saved.name.empty()) {
          macros_.erase(name);
        } else {
          macros_[name] = std::move(saved);
        }
      }
    }
  } else {
    // Dispatch to pragma handler (ignores unknown pragmas).
    dispatch_pragma(s, virtual_file_, 0);
  }
}

std::string Preprocessor::detect_include_guard(const std::string& source) {
  // Detect the pattern: #ifndef GUARD / #define GUARD / ... / #endif
  // Requires: first non-blank directive is #ifndef X, second is #define X,
  // and the file ends with #endif at the matching nesting level.
  std::istringstream in(source);
  std::string line;
  std::string guard_name;
  int directive_count = 0;
  int nesting = 0;
  bool valid = true;

  while (std::getline(in, line)) {
    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i >= line.size() || line[i] != '#') {
      // Non-directive line: only whitespace/comments allowed before the guard.
      if (directive_count == 0) {
        // Check if line is blank
        bool blank = true;
        for (size_t j = i; j < line.size(); ++j) {
          if (line[j] != ' ' && line[j] != '\t') { blank = false; break; }
        }
        if (!blank) { valid = false; break; }
      }
      continue;
    }
    // Parse directive
    ++i; // skip '#'
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    std::string key;
    while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
      key += line[i++];
    }
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    std::string rest = line.substr(i);
    // Trim trailing whitespace from rest
    while (!rest.empty() && (rest.back() == ' ' || rest.back() == '\t' || rest.back() == '\r'))
      rest.pop_back();

    if (directive_count == 0) {
      if (key != "ifndef") { valid = false; break; }
      guard_name = rest;
      if (guard_name.empty()) { valid = false; break; }
      // Guard name must be a single identifier
      for (char c : guard_name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
          valid = false; break;
        }
      }
      if (!valid) break;
      nesting = 1;
    } else if (directive_count == 1) {
      if (key != "define") { valid = false; break; }
      // Must define the same guard macro (possibly with a value)
      std::string def_name;
      size_t j = 0;
      while (j < rest.size() && rest[j] != ' ' && rest[j] != '\t' && rest[j] != '(') {
        def_name += rest[j++];
      }
      if (def_name != guard_name) { valid = false; break; }
    } else {
      // Track nesting
      if (key == "if" || key == "ifdef" || key == "ifndef") {
        ++nesting;
      } else if (key == "endif") {
        --nesting;
        if (nesting == 0) {
          // This must be the last directive — check remaining lines are blank.
          // We'll check below after the loop.
        }
      }
    }
    ++directive_count;
  }

  if (!valid || guard_name.empty() || nesting != 0) return {};
  if (directive_count < 2) return {};
  return guard_name;
}

std::string Preprocessor::get_builtin_header(const std::string& name) {
  if (name == "stdarg.h") {
    return
      "#ifndef _STDARG_H\n"
      "#define _STDARG_H\n"
      "typedef __builtin_va_list va_list;\n"
      "typedef __builtin_va_list __gnuc_va_list;\n"
      "#define va_start(ap, param) __builtin_va_start(ap, param)\n"
      "#define va_end(ap) __builtin_va_end(ap)\n"
      "#define va_arg(ap, type) __builtin_va_arg(ap, type)\n"
      "#define va_copy(dest, src) __builtin_va_copy(dest, src)\n"
      "#define __va_copy(dest, src) __builtin_va_copy(dest, src)\n"
      "#endif\n";
  }
  if (name == "limits.h") {
    return
      "#ifndef _LIMITS_H\n"
      "#define _LIMITS_H\n"
      "#define CHAR_BIT __CHAR_BIT__\n"
      "#define SCHAR_MIN (-__SCHAR_MAX__ - 1)\n"
      "#define SCHAR_MAX __SCHAR_MAX__\n"
      "#define UCHAR_MAX ((__SCHAR_MAX__ * 2) + 1)\n"
      "#define CHAR_MIN SCHAR_MIN\n"
      "#define CHAR_MAX SCHAR_MAX\n"
      "#define SHRT_MIN (-__SHRT_MAX__ - 1)\n"
      "#define SHRT_MAX __SHRT_MAX__\n"
      "#define USHRT_MAX ((__SHRT_MAX__ * 2) + 1)\n"
      "#define INT_MIN (-__INT_MAX__ - 1)\n"
      "#define INT_MAX __INT_MAX__\n"
      "#define UINT_MAX ((__INT_MAX__ * 2U) + 1U)\n"
      "#define LONG_MIN (-__LONG_MAX__ - 1L)\n"
      "#define LONG_MAX __LONG_MAX__\n"
      "#define ULONG_MAX ((__LONG_MAX__ * 2UL) + 1UL)\n"
      "#define LLONG_MIN (-__LONG_LONG_MAX__ - 1LL)\n"
      "#define LLONG_MAX __LONG_LONG_MAX__\n"
      "#define ULLONG_MAX ((__LONG_LONG_MAX__ * 2ULL) + 1ULL)\n"
      "#define LONG_LONG_MIN LLONG_MIN\n"
      "#define LONG_LONG_MAX LLONG_MAX\n"
      "#define ULONG_LONG_MAX ULLONG_MAX\n"
      "#define MB_LEN_MAX 16\n"
      "#endif\n";
  }
  if (name == "stddef.h") {
    return
      "#ifndef _STDDEF_H\n"
      "#define _STDDEF_H\n"
      "typedef __SIZE_TYPE__ size_t;\n"
      "typedef __PTRDIFF_TYPE__ ptrdiff_t;\n"
      "typedef int wchar_t;\n"
      "#define NULL ((void*)0)\n"
      "#define offsetof(type, member) __builtin_offsetof(type, member)\n"
      "#endif\n";
  }
  if (name == "stdbool.h") {
    return
      "#ifndef _STDBOOL_H\n"
      "#define _STDBOOL_H\n"
      "#define bool _Bool\n"
      "#define true 1\n"
      "#define false 0\n"
      "#define __bool_true_false_are_defined 1\n"
      "#endif\n";
  }
  if (name == "stdio.h") {
    return
      "#ifndef _STDIO_H\n"
      "#define _STDIO_H\n"
      "#ifndef NULL\n"
      "#define NULL ((void*)0)\n"
      "#endif\n"
      "typedef __SIZE_TYPE__ size_t;\n"
      "typedef struct _IO_FILE FILE;\n"
      "extern FILE *stdin;\n"
      "extern FILE *stdout;\n"
      "extern FILE *stderr;\n"
      "#define EOF (-1)\n"
      "#define SEEK_SET 0\n"
      "#define SEEK_CUR 1\n"
      "#define SEEK_END 2\n"
      "int printf(const char *, ...);\n"
      "int fprintf(FILE *, const char *, ...);\n"
      "int sprintf(char *, const char *, ...);\n"
      "int snprintf(char *, size_t, const char *, ...);\n"
      "int vprintf(const char *, __builtin_va_list);\n"
      "int vfprintf(FILE *, const char *, __builtin_va_list);\n"
      "int vsprintf(char *, const char *, __builtin_va_list);\n"
      "int vsnprintf(char *, size_t, const char *, __builtin_va_list);\n"
      "int scanf(const char *, ...);\n"
      "int fscanf(FILE *, const char *, ...);\n"
      "int sscanf(const char *, const char *, ...);\n"
      "int fputc(int, FILE *);\n"
      "int fputs(const char *, FILE *);\n"
      "int putc(int, FILE *);\n"
      "int putchar(int);\n"
      "int puts(const char *);\n"
      "int fgetc(FILE *);\n"
      "int getc(FILE *);\n"
      "int getchar(void);\n"
      "char *fgets(char *, int, FILE *);\n"
      "size_t fread(void *, size_t, size_t, FILE *);\n"
      "size_t fwrite(const void *, size_t, size_t, FILE *);\n"
      "FILE *fopen(const char *, const char *);\n"
      "int fclose(FILE *);\n"
      "int fflush(FILE *);\n"
      "int fseek(FILE *, long, int);\n"
      "long ftell(FILE *);\n"
      "void rewind(FILE *);\n"
      "int feof(FILE *);\n"
      "int ferror(FILE *);\n"
      "void perror(const char *);\n"
      "int remove(const char *);\n"
      "int rename(const char *, const char *);\n"
      "FILE *tmpfile(void);\n"
      "char *tmpnam(char *);\n"
      "FILE *freopen(const char *, const char *, FILE *);\n"
      "int setvbuf(FILE *, char *, int, size_t);\n"
      "void setbuf(FILE *, char *);\n"
      "int ungetc(int, FILE *);\n"
      "void clearerr(FILE *);\n"
      "#define _IOFBF 0\n"
      "#define _IOLBF 1\n"
      "#define _IONBF 2\n"
      "#define BUFSIZ 8192\n"
      "#endif\n";
  }
  if (name == "stdlib.h") {
    return
      "#ifndef _STDLIB_H\n"
      "#define _STDLIB_H\n"
      "#ifndef NULL\n"
      "#define NULL ((void*)0)\n"
      "#endif\n"
      "typedef __SIZE_TYPE__ size_t;\n"
      "#define EXIT_SUCCESS 0\n"
      "#define EXIT_FAILURE 1\n"
      "#define RAND_MAX 2147483647\n"
      "void abort(void);\n"
      "void exit(int);\n"
      "void _Exit(int);\n"
      "int atexit(void (*)(void));\n"
      "void *malloc(size_t);\n"
      "void *calloc(size_t, size_t);\n"
      "void *realloc(void *, size_t);\n"
      "void free(void *);\n"
      "int atoi(const char *);\n"
      "long atol(const char *);\n"
      "long long atoll(const char *);\n"
      "double atof(const char *);\n"
      "long strtol(const char *, char **, int);\n"
      "unsigned long strtoul(const char *, char **, int);\n"
      "long long strtoll(const char *, char **, int);\n"
      "unsigned long long strtoull(const char *, char **, int);\n"
      "double strtod(const char *, char **);\n"
      "int abs(int);\n"
      "long labs(long);\n"
      "long long llabs(long long);\n"
      "int rand(void);\n"
      "void srand(unsigned int);\n"
      "void qsort(void *, size_t, size_t, int (*)(const void *, const void *));\n"
      "void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));\n"
      "char *getenv(const char *);\n"
      "int system(const char *);\n"
      "#endif\n";
  }
  if (name == "string.h") {
    return
      "#ifndef _STRING_H\n"
      "#define _STRING_H\n"
      "#ifndef NULL\n"
      "#define NULL ((void*)0)\n"
      "#endif\n"
      "typedef __SIZE_TYPE__ size_t;\n"
      "void *memcpy(void *, const void *, size_t);\n"
      "void *memmove(void *, const void *, size_t);\n"
      "void *memset(void *, int, size_t);\n"
      "int memcmp(const void *, const void *, size_t);\n"
      "void *memchr(const void *, int, size_t);\n"
      "char *strcpy(char *, const char *);\n"
      "char *strncpy(char *, const char *, size_t);\n"
      "char *strcat(char *, const char *);\n"
      "char *strncat(char *, const char *, size_t);\n"
      "int strcmp(const char *, const char *);\n"
      "int strncmp(const char *, const char *, size_t);\n"
      "char *strchr(const char *, int);\n"
      "char *strrchr(const char *, int);\n"
      "char *strstr(const char *, const char *);\n"
      "size_t strlen(const char *);\n"
      "size_t strspn(const char *, const char *);\n"
      "size_t strcspn(const char *, const char *);\n"
      "char *strdup(const char *);\n"
      "char *strerror(int);\n"
      "#endif\n";
  }
  if (name == "signal.h") {
    return
      "#ifndef _SIGNAL_H\n"
      "#define _SIGNAL_H\n"
      "typedef void (*__sighandler_t)(int);\n"
      "#define SIG_DFL ((__sighandler_t)0)\n"
      "#define SIG_IGN ((__sighandler_t)1)\n"
      "#define SIG_ERR ((__sighandler_t)-1)\n"
      "#define SIGHUP 1\n"
      "#define SIGINT 2\n"
      "#define SIGQUIT 3\n"
      "#define SIGILL 4\n"
      "#define SIGTRAP 5\n"
      "#define SIGABRT 6\n"
      "#define SIGFPE 8\n"
      "#define SIGKILL 9\n"
      "#define SIGSEGV 11\n"
      "#define SIGPIPE 13\n"
      "#define SIGALRM 14\n"
      "#define SIGTERM 15\n"
      "__sighandler_t signal(int, __sighandler_t);\n"
      "int raise(int);\n"
      "#endif\n";
  }
  if (name == "assert.h") {
    return
      "#ifndef _ASSERT_H\n"
      "#define _ASSERT_H\n"
      "#ifdef NDEBUG\n"
      "#define assert(expr) ((void)0)\n"
      "#else\n"
      "extern void __assert_fail(const char *, const char *, unsigned int, const char *);\n"
      "#define assert(expr) ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))\n"
      "#endif\n"
      "#endif\n";
  }
  if (name == "stdbool.h") {
    return
      "#ifndef _STDBOOL_H\n"
      "#define _STDBOOL_H\n"
      "#define bool _Bool\n"
      "#define true 1\n"
      "#define false 0\n"
      "#define __bool_true_false_are_defined 1\n"
      "#endif\n";
  }
  if (name == "float.h") {
    return
      "#ifndef _FLOAT_H\n"
      "#define _FLOAT_H\n"
      "#define FLT_RADIX 2\n"
      "#define FLT_MANT_DIG 24\n"
      "#define FLT_DIG 6\n"
      "#define FLT_MIN_EXP (-125)\n"
      "#define FLT_MIN_10_EXP (-37)\n"
      "#define FLT_MAX_EXP 128\n"
      "#define FLT_MAX_10_EXP 38\n"
      "#define FLT_MAX 3.40282347e+38F\n"
      "#define FLT_MIN 1.17549435e-38F\n"
      "#define FLT_EPSILON 1.19209290e-07F\n"
      "#define DBL_MANT_DIG 53\n"
      "#define DBL_DIG 15\n"
      "#define DBL_MIN_EXP (-1021)\n"
      "#define DBL_MIN_10_EXP (-307)\n"
      "#define DBL_MAX_EXP 1024\n"
      "#define DBL_MAX_10_EXP 308\n"
      "#define DBL_MAX 1.7976931348623157e+308\n"
      "#define DBL_MIN 2.2250738585072014e-308\n"
      "#define DBL_EPSILON 2.2204460492503131e-16\n"
      "#define LDBL_MANT_DIG 64\n"
      "#define LDBL_DIG 18\n"
      "#define LDBL_MIN_EXP (-16381)\n"
      "#define LDBL_MIN_10_EXP (-4931)\n"
      "#define LDBL_MAX_EXP 16384\n"
      "#define LDBL_MAX_10_EXP 4932\n"
      "#define LDBL_MAX 1.18973149535723176502e+4932L\n"
      "#define LDBL_MIN 3.36210314311209350626e-4932L\n"
      "#define LDBL_EPSILON 1.08420217248550443401e-19L\n"
      "#define FLT_ROUNDS 1\n"
      "#define FLT_EVAL_METHOD 0\n"
      "#define DECIMAL_DIG 21\n"
      "#endif\n";
  }
  if (name == "setjmp.h") {
    return
      "#ifndef _SETJMP_H\n"
      "#define _SETJMP_H\n"
      "typedef long jmp_buf[8];\n"
      "typedef long sigjmp_buf[9];\n"
      "int setjmp(jmp_buf);\n"
      "void longjmp(jmp_buf, int);\n"
      "int _setjmp(jmp_buf);\n"
      "void _longjmp(jmp_buf, int);\n"
      "int sigsetjmp(sigjmp_buf, int);\n"
      "void siglongjmp(sigjmp_buf, int);\n"
      "#endif\n";
  }
  if (name == "ctype.h") {
    return
      "#ifndef _CTYPE_H\n"
      "#define _CTYPE_H\n"
      "int isalnum(int);\n"
      "int isalpha(int);\n"
      "int isblank(int);\n"
      "int iscntrl(int);\n"
      "int isdigit(int);\n"
      "int isgraph(int);\n"
      "int islower(int);\n"
      "int isprint(int);\n"
      "int ispunct(int);\n"
      "int isspace(int);\n"
      "int isupper(int);\n"
      "int isxdigit(int);\n"
      "int tolower(int);\n"
      "int toupper(int);\n"
      "#endif\n";
  }
  if (name == "math.h") {
    return
      "#ifndef _MATH_H\n"
      "#define _MATH_H\n"
      "#define HUGE_VAL __builtin_huge_val()\n"
      "#define HUGE_VALF __builtin_huge_valf()\n"
      "#define INFINITY __builtin_inff()\n"
      "#define NAN __builtin_nanf(\"\")\n"
      "#define isinf(x) __builtin_isinf(x)\n"
      "#define isnan(x) __builtin_isnan(x)\n"
      "#define isnormal(x) __builtin_isnormal(x)\n"
      "#define isfinite(x) __builtin_isfinite(x)\n"
      "#define signbit(x) __builtin_signbit(x)\n"
      "double fabs(double);\n"
      "float fabsf(float);\n"
      "double sqrt(double);\n"
      "float sqrtf(float);\n"
      "double sin(double);\n"
      "double cos(double);\n"
      "double tan(double);\n"
      "double asin(double);\n"
      "double acos(double);\n"
      "double atan(double);\n"
      "double atan2(double, double);\n"
      "double exp(double);\n"
      "double log(double);\n"
      "double log10(double);\n"
      "double pow(double, double);\n"
      "double ceil(double);\n"
      "double floor(double);\n"
      "double fmod(double, double);\n"
      "double ldexp(double, int);\n"
      "double frexp(double, int *);\n"
      "double modf(double, double *);\n"
      "float sinf(float);\n"
      "float cosf(float);\n"
      "float tanf(float);\n"
      "float sqrtf(float);\n"
      "float expf(float);\n"
      "float logf(float);\n"
      "float log10f(float);\n"
      "float powf(float, float);\n"
      "float ceilf(float);\n"
      "float floorf(float);\n"
      "float fmodf(float, float);\n"
      "#endif\n";
  }
  if (name == "fcntl.h") {
    return
      "#ifndef _FCNTL_H\n"
      "#define _FCNTL_H\n"
      "#define O_RDONLY 0\n"
      "#define O_WRONLY 1\n"
      "#define O_RDWR 2\n"
      "#define O_CREAT 0100\n"
      "#define O_EXCL 0200\n"
      "#define O_NOCTTY 0400\n"
      "#define O_TRUNC 01000\n"
      "#define O_APPEND 02000\n"
      "#define O_NONBLOCK 04000\n"
      "#define O_CLOEXEC 02000000\n"
      "int open(const char *, int, ...);\n"
      "int creat(const char *, unsigned int);\n"
      "int fcntl(int, int, ...);\n"
      "#endif\n";
  }
  if (name == "sys/mman.h") {
    return
      "#ifndef _SYS_MMAN_H\n"
      "#define _SYS_MMAN_H\n"
      "#ifndef NULL\n"
      "#define NULL ((void*)0)\n"
      "#endif\n"
      "typedef __SIZE_TYPE__ size_t;\n"
      "typedef long off_t;\n"
      "#define PROT_NONE 0\n"
      "#define PROT_READ 1\n"
      "#define PROT_WRITE 2\n"
      "#define PROT_EXEC 4\n"
      "#define MAP_SHARED 0x01\n"
      "#define MAP_PRIVATE 0x02\n"
      "#define MAP_FIXED 0x10\n"
      "#define MAP_ANONYMOUS 0x20\n"
      "#define MAP_ANON MAP_ANONYMOUS\n"
      "#define MAP_FAILED ((void *)-1)\n"
      "void *mmap(void *, size_t, int, int, int, off_t);\n"
      "int munmap(void *, size_t);\n"
      "int mprotect(void *, size_t, int);\n"
      "int msync(void *, size_t, int);\n"
      "#endif\n";
  }
  if (name == "sys/types.h") {
    return
      "#ifndef _SYS_TYPES_H\n"
      "#define _SYS_TYPES_H\n"
      "typedef __SIZE_TYPE__ size_t;\n"
      "typedef long ssize_t;\n"
      "typedef long off_t;\n"
      "typedef int pid_t;\n"
      "typedef unsigned int uid_t;\n"
      "typedef unsigned int gid_t;\n"
      "typedef unsigned int mode_t;\n"
      "#endif\n";
  }
  if (name == "sys/stat.h") {
    return
      "#ifndef _SYS_STAT_H\n"
      "#define _SYS_STAT_H\n"
      "typedef unsigned int mode_t;\n"
      "typedef long off_t;\n"
      "#define S_IRWXU 0700\n"
      "#define S_IRUSR 0400\n"
      "#define S_IWUSR 0200\n"
      "#define S_IXUSR 0100\n"
      "#define S_IRWXG 070\n"
      "#define S_IRGRP 040\n"
      "#define S_IWGRP 020\n"
      "#define S_IXGRP 010\n"
      "#define S_IRWXO 07\n"
      "#define S_IROTH 04\n"
      "#define S_IWOTH 02\n"
      "#define S_IXOTH 01\n"
      "int chmod(const char *, mode_t);\n"
      "int mkdir(const char *, mode_t);\n"
      "int stat(const char *, void *);\n"
      "int fstat(int, void *);\n"
      "#endif\n";
  }
  if (name == "unistd.h") {
    return
      "#ifndef _UNISTD_H\n"
      "#define _UNISTD_H\n"
      "typedef __SIZE_TYPE__ size_t;\n"
      "typedef long ssize_t;\n"
      "typedef long off_t;\n"
      "typedef int pid_t;\n"
      "#ifndef NULL\n"
      "#define NULL ((void*)0)\n"
      "#endif\n"
      "#define STDIN_FILENO 0\n"
      "#define STDOUT_FILENO 1\n"
      "#define STDERR_FILENO 2\n"
      "ssize_t read(int, void *, size_t);\n"
      "ssize_t write(int, const void *, size_t);\n"
      "int close(int);\n"
      "off_t lseek(int, off_t, int);\n"
      "int unlink(const char *);\n"
      "int rmdir(const char *);\n"
      "char *getcwd(char *, size_t);\n"
      "int chdir(const char *);\n"
      "pid_t fork(void);\n"
      "pid_t getpid(void);\n"
      "unsigned int sleep(unsigned int);\n"
      "int usleep(unsigned int);\n"
      "int access(const char *, int);\n"
      "#endif\n";
  }
  if (name == "stdint.h") {
    return
      "#ifndef _STDINT_H\n"
      "#define _STDINT_H\n"
      "typedef __INT8_TYPE__ int8_t;\n"
      "typedef __INT16_TYPE__ int16_t;\n"
      "typedef __INT32_TYPE__ int32_t;\n"
      "typedef __INT64_TYPE__ int64_t;\n"
      "typedef __UINT8_TYPE__ uint8_t;\n"
      "typedef __UINT16_TYPE__ uint16_t;\n"
      "typedef __UINT32_TYPE__ uint32_t;\n"
      "typedef __UINT64_TYPE__ uint64_t;\n"
      "typedef __INT_LEAST8_TYPE__ int_least8_t;\n"
      "typedef __INT_LEAST16_TYPE__ int_least16_t;\n"
      "typedef __INT_LEAST32_TYPE__ int_least32_t;\n"
      "typedef __INT_LEAST64_TYPE__ int_least64_t;\n"
      "typedef __UINT_LEAST8_TYPE__ uint_least8_t;\n"
      "typedef __UINT_LEAST16_TYPE__ uint_least16_t;\n"
      "typedef __UINT_LEAST32_TYPE__ uint_least32_t;\n"
      "typedef __UINT_LEAST64_TYPE__ uint_least64_t;\n"
      "typedef __INT_FAST8_TYPE__ int_fast8_t;\n"
      "typedef __INT_FAST16_TYPE__ int_fast16_t;\n"
      "typedef __INT_FAST32_TYPE__ int_fast32_t;\n"
      "typedef __INT_FAST64_TYPE__ int_fast64_t;\n"
      "typedef __UINT_FAST8_TYPE__ uint_fast8_t;\n"
      "typedef __UINT_FAST16_TYPE__ uint_fast16_t;\n"
      "typedef __UINT_FAST32_TYPE__ uint_fast32_t;\n"
      "typedef __UINT_FAST64_TYPE__ uint_fast64_t;\n"
      "typedef __INTPTR_TYPE__ intptr_t;\n"
      "typedef __UINTPTR_TYPE__ uintptr_t;\n"
      "typedef __INTMAX_TYPE__ intmax_t;\n"
      "typedef __UINTMAX_TYPE__ uintmax_t;\n"
      "#define INT8_MIN (-__INT8_MAX__ - 1)\n"
      "#define INT8_MAX __INT8_MAX__\n"
      "#define UINT8_MAX __UINT8_MAX__\n"
      "#define INT16_MIN (-__INT16_MAX__ - 1)\n"
      "#define INT16_MAX __INT16_MAX__\n"
      "#define UINT16_MAX __UINT16_MAX__\n"
      "#define INT32_MIN (-__INT32_MAX__ - 1)\n"
      "#define INT32_MAX __INT32_MAX__\n"
      "#define UINT32_MAX __UINT32_MAX__\n"
      "#define INT64_MIN (-__INT64_MAX__ - 1LL)\n"
      "#define INT64_MAX __INT64_MAX__\n"
      "#define UINT64_MAX __UINT64_MAX__\n"
      "#define INTPTR_MIN (-__INTPTR_MAX__ - 1)\n"
      "#define INTPTR_MAX __INTPTR_MAX__\n"
      "#define UINTPTR_MAX __UINTPTR_MAX__\n"
      "#define INTMAX_MIN (-__INTMAX_MAX__ - 1LL)\n"
      "#define INTMAX_MAX __INTMAX_MAX__\n"
      "#define UINTMAX_MAX __UINTMAX_MAX__\n"
      "#define PTRDIFF_MIN (-__PTRDIFF_MAX__ - 1)\n"
      "#define PTRDIFF_MAX __PTRDIFF_MAX__\n"
      "#define SIZE_MAX __SIZE_MAX__\n"
      "#define INT8_C(x) (x)\n"
      "#define INT16_C(x) (x)\n"
      "#define INT32_C(x) (x)\n"
      "#define INT64_C(x) (x ## LL)\n"
      "#define UINT8_C(x) (x)\n"
      "#define UINT16_C(x) (x)\n"
      "#define UINT32_C(x) (x ## U)\n"
      "#define UINT64_C(x) (x ## ULL)\n"
      "#define INTMAX_C(x) (x ## LL)\n"
      "#define UINTMAX_C(x) (x ## ULL)\n"
      "#endif\n";
  }
  if (name == "inttypes.h") {
    return
      "#ifndef _INTTYPES_H\n"
      "#define _INTTYPES_H\n"
      "#include <stdint.h>\n"
      "typedef struct { intmax_t quot; intmax_t rem; } imaxdiv_t;\n"
      "intmax_t imaxabs(intmax_t);\n"
      "imaxdiv_t imaxdiv(intmax_t, intmax_t);\n"
      "intmax_t strtoimax(const char *, char **, int);\n"
      "uintmax_t strtoumax(const char *, char **, int);\n"
      "#define PRId8 \"d\"\n"
      "#define PRId16 \"d\"\n"
      "#define PRId32 \"d\"\n"
      "#define PRId64 \"ld\"\n"
      "#define PRIi8 \"i\"\n"
      "#define PRIi16 \"i\"\n"
      "#define PRIi32 \"i\"\n"
      "#define PRIi64 \"li\"\n"
      "#define PRIu8 \"u\"\n"
      "#define PRIu16 \"u\"\n"
      "#define PRIu32 \"u\"\n"
      "#define PRIu64 \"lu\"\n"
      "#define PRIx8 \"x\"\n"
      "#define PRIx16 \"x\"\n"
      "#define PRIx32 \"x\"\n"
      "#define PRIx64 \"lx\"\n"
      "#define PRIX8 \"X\"\n"
      "#define PRIX16 \"X\"\n"
      "#define PRIX32 \"X\"\n"
      "#define PRIX64 \"lX\"\n"
      "#define PRIo8 \"o\"\n"
      "#define PRIo16 \"o\"\n"
      "#define PRIo32 \"o\"\n"
      "#define PRIo64 \"lo\"\n"
      "#define SCNd32 \"d\"\n"
      "#define SCNd64 \"ld\"\n"
      "#define SCNi32 \"i\"\n"
      "#define SCNi64 \"li\"\n"
      "#define SCNu32 \"u\"\n"
      "#define SCNu64 \"lu\"\n"
      "#define SCNx32 \"x\"\n"
      "#define SCNx64 \"lx\"\n"
      "#endif\n";
  }
  if (name == "errno.h") {
    return
      "#ifndef _ERRNO_H\n"
      "#define _ERRNO_H\n"
      "extern int *__errno_location(void);\n"
      "#define errno (*__errno_location())\n"
      "#define EPERM 1\n"
      "#define ENOENT 2\n"
      "#define ESRCH 3\n"
      "#define EINTR 4\n"
      "#define EIO 5\n"
      "#define ENXIO 6\n"
      "#define E2BIG 7\n"
      "#define ENOEXEC 8\n"
      "#define EBADF 9\n"
      "#define ECHILD 10\n"
      "#define EAGAIN 11\n"
      "#define ENOMEM 12\n"
      "#define EACCES 13\n"
      "#define EFAULT 14\n"
      "#define EBUSY 16\n"
      "#define EEXIST 17\n"
      "#define EXDEV 18\n"
      "#define ENODEV 19\n"
      "#define ENOTDIR 20\n"
      "#define EISDIR 21\n"
      "#define EINVAL 22\n"
      "#define ENFILE 23\n"
      "#define EMFILE 24\n"
      "#define ENOTTY 25\n"
      "#define EFBIG 27\n"
      "#define ENOSPC 28\n"
      "#define ESPIPE 29\n"
      "#define EROFS 30\n"
      "#define EMLINK 31\n"
      "#define EPIPE 32\n"
      "#define EDOM 33\n"
      "#define ERANGE 34\n"
      "#define EDEADLK 35\n"
      "#define ENAMETOOLONG 36\n"
      "#define ENOSYS 38\n"
      "#define ENOTEMPTY 39\n"
      "#define EWOULDBLOCK EAGAIN\n"
      "#endif\n";
  }
  if (name == "time.h") {
    return
      "#ifndef _TIME_H\n"
      "#define _TIME_H\n"
      "#ifndef NULL\n"
      "#define NULL ((void*)0)\n"
      "#endif\n"
      "typedef __SIZE_TYPE__ size_t;\n"
      "typedef long time_t;\n"
      "typedef long clock_t;\n"
      "#define CLOCKS_PER_SEC 1000000L\n"
      "struct tm {\n"
      "  int tm_sec;\n"
      "  int tm_min;\n"
      "  int tm_hour;\n"
      "  int tm_mday;\n"
      "  int tm_mon;\n"
      "  int tm_year;\n"
      "  int tm_wday;\n"
      "  int tm_yday;\n"
      "  int tm_isdst;\n"
      "};\n"
      "time_t time(time_t *);\n"
      "clock_t clock(void);\n"
      "double difftime(time_t, time_t);\n"
      "time_t mktime(struct tm *);\n"
      "struct tm *localtime(const time_t *);\n"
      "struct tm *gmtime(const time_t *);\n"
      "char *asctime(const struct tm *);\n"
      "char *ctime(const time_t *);\n"
      "size_t strftime(char *, size_t, const char *, const struct tm *);\n"
      "#endif\n";
  }
  return {};
}

}  // namespace tinyc2ll::frontend_cxx
