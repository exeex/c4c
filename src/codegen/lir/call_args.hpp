#pragma once

#include "operands.hpp"

#include <array>
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::codegen::lir {

enum class LirCallCalleeKind : unsigned char {
  DirectGlobal,
  DirectIntrinsic,
  Indirect,
};

struct ParsedLirCallCalleeView {
  LirCallCalleeKind kind = LirCallCalleeKind::Indirect;
  std::string_view symbol_name;
};

struct LirTypedCallArgView {
  std::string_view type;
  std::string_view operand;
};

struct ParsedLirTypedCallView {
  std::vector<std::string_view> param_types;
  std::vector<LirTypedCallArgView> args;
};

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

inline std::optional<ParsedLirCallCalleeView> parse_lir_call_callee(
    std::string_view callee) {
  callee = trim_lir_arg_text(callee);
  if (callee.empty()) {
    return std::nullopt;
  }
  if (callee.front() == '@') {
    const auto symbol_name = callee.substr(1);
    if (symbol_name.empty()) {
      return std::nullopt;
    }
    const auto kind = symbol_name.rfind("llvm.", 0) == 0
                          ? LirCallCalleeKind::DirectIntrinsic
                          : LirCallCalleeKind::DirectGlobal;
    return ParsedLirCallCalleeView{kind, symbol_name};
  }
  if (callee.front() == '%') {
    return ParsedLirCallCalleeView{LirCallCalleeKind::Indirect, {}};
  }
  return std::nullopt;
}

inline std::optional<std::string_view> parse_lir_direct_global_callee(
    std::string_view callee) {
  const auto parsed = parse_lir_call_callee(callee);
  if (!parsed.has_value() || parsed->kind != LirCallCalleeKind::DirectGlobal) {
    return std::nullopt;
  }
  return parsed->symbol_name;
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
inline void for_each_lir_top_level_segment(std::string_view text,
                                           char delim,
                                           Fn&& fn) {
  std::size_t start = 0;
  int paren_depth = 0;
  int bracket_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;

  auto emit_arg = [&](std::size_t end) {
    const auto segment = trim_lir_arg_text(text.substr(start, end - start));
    if (!segment.empty()) {
      fn(segment);
    }
  };

  for (std::size_t index = 0; index < text.size(); ++index) {
    switch (text[index]) {
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
      default:
        break;
    }
    if (text[index] == delim && paren_depth == 0 && bracket_depth == 0 &&
        brace_depth == 0 && angle_depth == 0) {
      emit_arg(index);
      start = index + 1;
    }
  }

  emit_arg(text.size());
}

template <typename Fn>
inline void for_each_lir_call_arg(std::string_view args_str, Fn&& fn) {
  for_each_lir_top_level_segment(args_str, ',', std::forward<Fn>(fn));
}

inline std::optional<LirTypedCallArgView> parse_lir_typed_call_arg(std::string_view arg);

inline std::optional<std::vector<LirTypedCallArgView>> parse_lir_typed_call_args(
    std::string_view args_str) {
  std::vector<LirTypedCallArgView> args;
  bool parse_failed = false;
  for_each_lir_call_arg(args_str, [&](std::string_view arg) {
    const auto parsed = parse_lir_typed_call_arg(arg);
    if (!parsed.has_value()) {
      parse_failed = true;
      return;
    }
    args.push_back(*parsed);
  });
  if (parse_failed) {
    return std::nullopt;
  }
  return args;
}

inline std::optional<std::vector<std::string_view>> parse_lir_call_param_types(
    std::string_view callee_type_suffix) {
  callee_type_suffix = trim_lir_arg_text(callee_type_suffix);
  if (callee_type_suffix.empty()) {
    return std::vector<std::string_view>{};
  }
  if (callee_type_suffix.size() < 2 || callee_type_suffix.front() != '(' ||
      callee_type_suffix.back() != ')') {
    return std::nullopt;
  }

  const auto inner = trim_lir_arg_text(
      callee_type_suffix.substr(1, callee_type_suffix.size() - 2));
  std::vector<std::string_view> param_types;
  if (inner.empty()) {
    return param_types;
  }

  bool parse_failed = false;
  for_each_lir_top_level_segment(inner, ',', [&](std::string_view part) {
    if (part.empty()) {
      parse_failed = true;
      return;
    }
    param_types.push_back(part);
  });
  if (parse_failed) {
    return std::nullopt;
  }
  return param_types;
}

inline std::optional<ParsedLirTypedCallView> parse_lir_typed_call(
    std::string_view callee_type_suffix,
    std::string_view args_str) {
  const auto param_types = parse_lir_call_param_types(callee_type_suffix);
  const auto args = parse_lir_typed_call_args(args_str);
  if (!param_types.has_value() || !args.has_value() ||
      param_types->size() != args->size()) {
    return std::nullopt;
  }

  for (std::size_t index = 0; index < args->size(); ++index) {
    if ((*param_types)[index] != (*args)[index].type) {
      return std::nullopt;
    }
  }

  return ParsedLirTypedCallView{*param_types, *args};
}

template <std::size_t N>
inline bool lir_typed_call_has_param_types(
    const ParsedLirTypedCallView& parsed,
    const std::array<std::string_view, N>& expected_types) {
  if (parsed.param_types.size() != expected_types.size()) {
    return false;
  }
  for (std::size_t index = 0; index < expected_types.size(); ++index) {
    if (parsed.param_types[index] != expected_types[index]) {
      return false;
    }
  }
  return true;
}

inline std::optional<std::string_view> parse_lir_single_typed_call_operand(
    std::string_view callee_type_suffix,
    std::string_view args_str,
    std::string_view expected_type) {
  const auto parsed = parse_lir_typed_call(callee_type_suffix, args_str);
  if (!parsed.has_value() ||
      !lir_typed_call_has_param_types(*parsed, std::array<std::string_view, 1>{expected_type})) {
    return std::nullopt;
  }
  return parsed->args.front().operand;
}

inline std::optional<std::pair<std::string_view, std::string_view>>
parse_lir_two_typed_call_operands(std::string_view callee_type_suffix,
                                  std::string_view args_str,
                                  std::string_view expected_type0,
                                  std::string_view expected_type1) {
  const auto parsed = parse_lir_typed_call(callee_type_suffix, args_str);
  if (!parsed.has_value() ||
      !lir_typed_call_has_param_types(
          *parsed, std::array<std::string_view, 2>{expected_type0, expected_type1})) {
    return std::nullopt;
  }
  return std::pair<std::string_view, std::string_view>{parsed->args[0].operand,
                                                       parsed->args[1].operand};
}

inline bool lir_call_has_no_args(std::string_view callee_type_suffix,
                                 std::string_view args_str) {
  const auto param_types = parse_lir_call_param_types(callee_type_suffix);
  if (!param_types.has_value() || !param_types->empty()) {
    return false;
  }

  return trim_lir_arg_text(args_str).empty();
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

inline std::optional<LirTypedCallArgView> parse_lir_typed_call_arg(std::string_view arg) {
  arg = trim_lir_arg_text(arg);
  if (arg.empty()) {
    return std::nullopt;
  }

  const auto operand = lir_call_arg_operand(arg);
  if (!operand.has_value()) {
    return std::nullopt;
  }

  const auto operand_offset = static_cast<std::size_t>(operand->data() - arg.data());
  const auto type = trim_lir_arg_text(arg.substr(0, operand_offset));
  if (type.empty()) {
    return std::nullopt;
  }

  return LirTypedCallArgView{type, *operand};
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
