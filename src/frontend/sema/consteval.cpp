#include "consteval.hpp"

#include <cctype>
#include <cstdint>
#include <cstring>

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {
namespace {

std::string decode_c_escaped_bytes_local(const std::string& raw) {
  std::string out;
  for (size_t i = 0; i < raw.size();) {
    const unsigned char c = static_cast<unsigned char>(raw[i]);
    if (c != '\\') {
      out.push_back(static_cast<char>(c));
      ++i;
      continue;
    }
    ++i;
    if (i >= raw.size()) break;
    const char esc = raw[i++];
    switch (esc) {
      case 'n': out.push_back('\n'); break;
      case 't': out.push_back('\t'); break;
      case 'r': out.push_back('\r'); break;
      case 'a': out.push_back('\a'); break;
      case 'b': out.push_back('\b'); break;
      case 'f': out.push_back('\f'); break;
      case 'v': out.push_back('\v'); break;
      case '\\': out.push_back('\\'); break;
      case '\'': out.push_back('\''); break;
      case '"': out.push_back('"'); break;
      case '?': out.push_back('\?'); break;
      case 'x': {
        unsigned value = 0;
        while (i < raw.size() && std::isxdigit(static_cast<unsigned char>(raw[i]))) {
          const unsigned char h = static_cast<unsigned char>(raw[i++]);
          value = value * 16 + (std::isdigit(h) ? h - '0' : std::tolower(h) - 'a' + 10);
        }
        out.push_back(static_cast<char>(value & 0xFF));
        break;
      }
      default:
        if (esc >= '0' && esc <= '7') {
          unsigned value = static_cast<unsigned>(esc - '0');
          for (int k = 0; k < 2 && i < raw.size() && raw[i] >= '0' && raw[i] <= '7'; ++k, ++i) {
            value = value * 8 + static_cast<unsigned>(raw[i] - '0');
          }
          out.push_back(static_cast<char>(value & 0xFF));
        } else {
          out.push_back(esc);
        }
        break;
    }
  }
  return out;
}

}  // namespace

std::vector<long long> decode_string_literal_values(const char* sval, bool wide) {
  std::vector<long long> out;
  if (!sval) {
    out.push_back(0);
    return out;
  }
  const char* p = sval;
  while (*p && *p != '"') ++p;
  if (*p == '"') ++p;
  while (*p && *p != '"') {
    if (*p == '\\' && *(p + 1)) {
      ++p;
      switch (*p) {
        case 'n': out.push_back('\n'); ++p; break;
        case 't': out.push_back('\t'); ++p; break;
        case 'r': out.push_back('\r'); ++p; break;
        case '0': out.push_back(0); ++p; break;
        case '\\': out.push_back('\\'); ++p; break;
        case '\'': out.push_back('\''); ++p; break;
        case '"': out.push_back('"'); ++p; break;
        default: out.push_back(static_cast<unsigned char>(*p)); ++p; break;
      }
      continue;
    }

    const unsigned char c0 = static_cast<unsigned char>(*p);
    if (!wide || c0 < 0x80) {
      out.push_back(static_cast<long long>(c0));
      ++p;
      continue;
    }
    if ((c0 & 0xE0) == 0xC0 && p[1]) {
      const uint32_t cp = ((c0 & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 2;
      continue;
    }
    if ((c0 & 0xF0) == 0xE0 && p[1] && p[2]) {
      const uint32_t cp = ((c0 & 0x0F) << 12) |
                          ((static_cast<unsigned char>(p[1]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(p[2]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 3;
      continue;
    }
    if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
      const uint32_t cp = ((c0 & 0x07) << 18) |
                          ((static_cast<unsigned char>(p[1]) & 0x3F) << 12) |
                          ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(p[3]) & 0x3F);
      out.push_back(static_cast<long long>(cp));
      p += 4;
      continue;
    }
    out.push_back(static_cast<long long>(c0));
    ++p;
  }
  out.push_back(0);
  return out;
}

std::string bytes_from_string_literal(const StringLiteral& sl) {
  std::string bytes = sl.raw;
  if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
    bytes = bytes.substr(1, bytes.size() - 2);
  } else if (bytes.size() >= 3 && bytes[0] == 'L' && bytes[1] == '"' && bytes.back() == '"') {
    bytes = bytes.substr(2, bytes.size() - 3);
  }
  return decode_c_escaped_bytes_local(bytes);
}

std::optional<long long> eval_int_const_expr(
    const Node* n,
    const std::unordered_map<std::string, long long>& enum_consts) {
  if (!n) return std::nullopt;
  switch (n->kind) {
    case NK_INT_LIT:
    case NK_CHAR_LIT:
      return n->ival;
    case NK_VAR: {
      if (n->name && n->name[0]) {
        auto it = enum_consts.find(n->name);
        if (it != enum_consts.end()) return it->second;
      }
      return std::nullopt;
    }
    case NK_CAST:
      return eval_int_const_expr(n->left, enum_consts);
    case NK_UNARY: {
      auto v = eval_int_const_expr(n->left, enum_consts);
      if (!v || !n->op) return std::nullopt;
      if (std::strcmp(n->op, "+") == 0) return *v;
      if (std::strcmp(n->op, "-") == 0) return -*v;
      if (std::strcmp(n->op, "~") == 0) return ~*v;
      if (std::strcmp(n->op, "!") == 0) return static_cast<long long>(!*v);
      return std::nullopt;
    }
    case NK_BINOP: {
      auto l = eval_int_const_expr(n->left, enum_consts);
      auto r = eval_int_const_expr(n->right, enum_consts);
      if (!l || !r || !n->op) return std::nullopt;
      if (std::strcmp(n->op, "+") == 0) return *l + *r;
      if (std::strcmp(n->op, "-") == 0) return *l - *r;
      if (std::strcmp(n->op, "*") == 0) return *l * *r;
      if (std::strcmp(n->op, "/") == 0) return (*r == 0) ? 0LL : (*l / *r);
      if (std::strcmp(n->op, "%") == 0) return (*r == 0) ? 0LL : (*l % *r);
      if (std::strcmp(n->op, "<<") == 0) return *l << *r;
      if (std::strcmp(n->op, ">>") == 0) return *l >> *r;
      if (std::strcmp(n->op, "&") == 0) return *l & *r;
      if (std::strcmp(n->op, "|") == 0) return *l | *r;
      if (std::strcmp(n->op, "^") == 0) return *l ^ *r;
      if (std::strcmp(n->op, "<") == 0) return static_cast<long long>(*l < *r);
      if (std::strcmp(n->op, "<=") == 0) return static_cast<long long>(*l <= *r);
      if (std::strcmp(n->op, ">") == 0) return static_cast<long long>(*l > *r);
      if (std::strcmp(n->op, ">=") == 0) return static_cast<long long>(*l >= *r);
      if (std::strcmp(n->op, "==") == 0) return static_cast<long long>(*l == *r);
      if (std::strcmp(n->op, "!=") == 0) return static_cast<long long>(*l != *r);
      if (std::strcmp(n->op, "&&") == 0) return static_cast<long long>(*l && *r);
      if (std::strcmp(n->op, "||") == 0) return static_cast<long long>(*l || *r);
      return std::nullopt;
    }
    case NK_TERNARY: {
      auto c = eval_int_const_expr(n->cond ? n->cond : n->left, enum_consts);
      if (!c) return std::nullopt;
      return eval_int_const_expr(*c ? n->then_ : n->else_, enum_consts);
    }
    default:
      return std::nullopt;
  }
}

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
