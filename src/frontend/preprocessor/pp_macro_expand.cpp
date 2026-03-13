#include "pp_macro_expand.hpp"
#include "pp_text.hpp"

#include <cctype>

namespace tinyc2ll::frontend_cxx {

std::vector<std::string> collect_funclike_args(const std::string& line, size_t* pos) {
  std::vector<std::string> args;
  std::string cur;
  int depth = 0;
  bool in_str = false, in_chr = false;

  while (*pos < line.size()) {
    char c = line[*pos];
    if (!in_str && !in_chr) {
      if (c == '(') { ++depth; cur.push_back(c); ++(*pos); continue; }
      if (c == ')') {
        if (depth == 0) {
          args.push_back(cur);
          ++(*pos);
          return args;
        }
        --depth; cur.push_back(c); ++(*pos); continue;
      }
      if (c == ',' && depth == 0) {
        args.push_back(cur);
        cur.clear();
        ++(*pos);
        continue;
      }
    }
    if (!in_chr && c == '"') {
      in_str = !in_str;
    } else if (!in_str && c == '\'') {
      in_chr = !in_chr;
    }
    if (in_str || in_chr) {
      cur.push_back(c);
      ++(*pos);
      if (c == '\\' && *pos < line.size()) {
        cur.push_back(line[*pos]);
        ++(*pos);
      }
      if (*pos < line.size()) {
        char q = line[*pos];
        if ((in_str && q == '"') || (in_chr && q == '\'')) {
          cur.push_back(q);
          ++(*pos);
          if (in_str) in_str = false;
          else in_chr = false;
        }
      }
      continue;
    }
    cur.push_back(c);
    ++(*pos);
  }
  if (!cur.empty() || !args.empty()) args.push_back(cur);
  return args;
}

std::string stringify_arg(const std::string& raw) {
  std::string r = trim_copy(raw);
  std::string out;
  out.push_back('"');
  for (char c : r) {
    if (c == '\\' || c == '"') out.push_back('\\');
    out.push_back(c);
  }
  out.push_back('"');
  return out;
}

int find_param_idx(const std::vector<std::string>& params, const std::string& name) {
  for (int i = 0; i < static_cast<int>(params.size()); ++i) {
    if (params[i] == name) return i;
  }
  return -1;
}

std::string substitute_funclike_body(const std::string& body,
                                     const std::vector<std::string>& params,
                                     const std::vector<std::string>& raw_args,
                                     const std::vector<std::string>& exp_args,
                                     bool variadic,
                                     const std::string& va_raw,
                                     const std::string& va_exp) {
  std::string out;
  size_t i = 0;

  auto get_raw = [&](const std::string& name) -> std::string {
    if (variadic && name == "__VA_ARGS__") return va_raw;
    int idx = find_param_idx(params, name);
    return (idx >= 0 && idx < static_cast<int>(raw_args.size())) ? trim_copy(raw_args[idx]) : name;
  };
  auto get_exp = [&](const std::string& name) -> std::string {
    if (variadic && name == "__VA_ARGS__") return va_exp;
    int idx = find_param_idx(params, name);
    return (idx >= 0 && idx < static_cast<int>(exp_args.size())) ? exp_args[idx] : name;
  };

  while (i < body.size()) {
    // String literal in body — copy verbatim
    if (body[i] == '"') {
      out.push_back('"');
      ++i;
      while (i < body.size() && body[i] != '"') {
        if (body[i] == '\\') { out.push_back(body[i++]); }
        if (i < body.size()) out.push_back(body[i++]);
      }
      if (i < body.size()) { out.push_back('"'); ++i; }
      continue;
    }

    // # operator (stringify)
    if (body[i] == '#' && i + 1 < body.size() && body[i + 1] != '#') {
      size_t j = i + 1;
      while (j < body.size() && (body[j] == ' ' || body[j] == '\t')) ++j;
      if (j < body.size() && is_ident_start(body[j])) {
        size_t k = j + 1;
        while (k < body.size() && is_ident_continue(body[k])) ++k;
        std::string pname = body.substr(j, k - j);
        bool is_param = (find_param_idx(params, pname) >= 0) ||
                        (variadic && pname == "__VA_ARGS__");
        if (is_param) {
          out += stringify_arg(get_raw(pname));
          i = k;
          continue;
        }
      }
      out.push_back('#');
      ++i;
      continue;
    }

    // Identifier — check for ## paste or parameter substitution
    if (is_ident_start(body[i])) {
      size_t j = i + 1;
      while (j < body.size() && is_ident_continue(body[j])) ++j;
      std::string tok = body.substr(i, j - i);

      // Check: is this identifier a character/string literal prefix?
      if (j < body.size() && (body[j] == '\'' || body[j] == '"')) {
        out += tok;
        char delim = body[j];
        out.push_back(body[j++]);
        while (j < body.size() && body[j] != delim) {
          if (body[j] == '\\') { out.push_back(body[j++]); }
          if (j < body.size()) out.push_back(body[j++]);
        }
        if (j < body.size()) { out.push_back(body[j++]); }
        i = j;
        continue;
      }

      // Is there ## following this token?
      size_t k = j;
      while (k < body.size() && (body[k] == ' ' || body[k] == '\t')) ++k;
      if (k + 1 < body.size() && body[k] == '#' && body[k + 1] == '#') {
        std::string left = get_raw(tok);
        out += trim_copy(left);
        k += 2;
        while (k < body.size() && (body[k] == ' ' || body[k] == '\t')) ++k;
        if (k < body.size() && is_ident_start(body[k])) {
          size_t m = k + 1;
          while (m < body.size() && is_ident_continue(body[m])) ++m;
          std::string rtok = body.substr(k, m - k);
          out += get_raw(rtok);
          i = m;
        } else {
          if (k < body.size()) { out.push_back(body[k]); i = k + 1; }
          else i = k;
        }
        continue;
      }

      // Normal parameter substitution (use pre-scan expanded arg)
      out += get_exp(tok);
      i = j;
      continue;
    }

    // ## without left-operand identifier
    if (body[i] == '#' && i + 1 < body.size() && body[i + 1] == '#') {
      while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) out.pop_back();
      i += 2;
      while (i < body.size() && (body[i] == ' ' || body[i] == '\t')) ++i;
      if (i < body.size() && is_ident_start(body[i])) {
        size_t j = i + 1;
        while (j < body.size() && is_ident_continue(body[j])) ++j;
        std::string rtok = body.substr(i, j - i);
        out += get_raw(rtok);
        i = j;
      } else if (i < body.size()) {
        out.push_back(body[i++]);
      }
      continue;
    }

    out.push_back(body[i++]);
  }

  return out;
}

std::pair<std::string, std::string> split_directive(const std::string& line) {
  size_t i = 0;
  while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
  size_t k0 = i;
  while (i < line.size() && is_ident_continue(line[i])) ++i;
  std::string key = line.substr(k0, i - k0);
  while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
  std::string rest = (i < line.size()) ? line.substr(i) : std::string();
  return {key, rest};
}

}  // namespace tinyc2ll::frontend_cxx
