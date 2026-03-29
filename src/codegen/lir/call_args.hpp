#pragma once

#include "operands.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::codegen::lir {

inline std::string_view trim_lir_arg_text(std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
    text.remove_prefix(1);
  }
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) {
    text.remove_suffix(1);
  }
  return text;
}

inline bool is_lir_value_name_char(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.' ||
         ch == '-';
}

inline bool is_lir_value_name(std::string_view token) {
  return !token.empty() && token.front() == '%';
}

inline void append_unique_lir_value_name(std::vector<std::string>& values,
                                         std::string value) {
  if (value.empty()) {
    return;
  }
  if (std::find(values.begin(), values.end(), value) == values.end()) {
    values.push_back(std::move(value));
  }
}

inline void collect_lir_value_names_from_text(std::string_view text,
                                              std::vector<std::string>& values) {
  std::size_t pos = 0;
  while (pos < text.size()) {
    pos = text.find('%', pos);
    if (pos == std::string_view::npos) {
      return;
    }
    std::size_t end = pos + 1;
    while (end < text.size() && is_lir_value_name_char(text[end])) {
      ++end;
    }
    if (end > pos + 1) {
      append_unique_lir_value_name(values, std::string(text.substr(pos, end - pos)));
    }
    pos = end;
  }
}

template <typename Fn>
inline void for_each_lir_call_arg(std::string_view args_str, Fn&& fn) {
  std::size_t start = 0;
  int paren_depth = 0;
  int bracket_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;

  auto emit_arg = [&](std::size_t end) {
    const auto arg = trim_lir_arg_text(args_str.substr(start, end - start));
    if (!arg.empty()) {
      fn(arg);
    }
  };

  for (std::size_t index = 0; index < args_str.size(); ++index) {
    switch (args_str[index]) {
      case '(':
        ++paren_depth;
        break;
      case ')':
        if (paren_depth > 0) --paren_depth;
        break;
      case '[':
        ++bracket_depth;
        break;
      case ']':
        if (bracket_depth > 0) --bracket_depth;
        break;
      case '{':
        ++brace_depth;
        break;
      case '}':
        if (brace_depth > 0) --brace_depth;
        break;
      case '<':
        ++angle_depth;
        break;
      case '>':
        if (angle_depth > 0) --angle_depth;
        break;
      case ',':
        if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0 &&
            angle_depth == 0) {
          emit_arg(index);
          start = index + 1;
        }
        break;
      default:
        break;
    }
  }

  emit_arg(args_str.size());
}

inline std::optional<std::string_view> lir_call_arg_operand(std::string_view arg) {
  arg = trim_lir_arg_text(arg);
  if (arg.empty()) {
    return std::nullopt;
  }

  std::optional<std::size_t> split_pos;
  int paren_depth = 0;
  int bracket_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;

  for (std::size_t index = 0; index < arg.size(); ++index) {
    switch (arg[index]) {
      case '(':
        ++paren_depth;
        break;
      case ')':
        if (paren_depth > 0) --paren_depth;
        break;
      case '[':
        ++bracket_depth;
        break;
      case ']':
        if (bracket_depth > 0) --bracket_depth;
        break;
      case '{':
        ++brace_depth;
        break;
      case '}':
        if (brace_depth > 0) --brace_depth;
        break;
      case '<':
        ++angle_depth;
        break;
      case '>':
        if (angle_depth > 0) --angle_depth;
        break;
      case ' ':
      case '\t':
        if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0 &&
            angle_depth == 0) {
          split_pos = index;
        }
        break;
      default:
        break;
    }
  }

  if (!split_pos.has_value()) {
    return arg;
  }
  const auto operand = trim_lir_arg_text(arg.substr(*split_pos + 1));
  if (operand.empty()) {
    return std::nullopt;
  }
  return operand;
}

inline void collect_lir_value_names_from_call_args(std::string_view args_str,
                                                   std::vector<std::string>& values) {
  for_each_lir_call_arg(args_str, [&](std::string_view arg) {
    const auto operand = lir_call_arg_operand(arg);
    if (operand.has_value() && is_lir_value_name(*operand)) {
      append_unique_lir_value_name(values, std::string(*operand));
      return;
    }
    collect_lir_value_names_from_text(arg, values);
  });
}

}  // namespace c4c::codegen::lir
