#pragma once

// Exported LIR call-argument model helpers.
//
// This header is intentionally independent of `LirCallOp`: include it when a
// caller needs to parse, format, or own typed call argument text without pulling
// in the full LIR IR index.

#include "operands.hpp"
#include "types.hpp"

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

struct OwnedLirTypedCallArg {
  std::string type;
  std::string operand;
  LirTypeRef type_ref;
};

struct FormattedLirTypedCall {
  std::string callee_type_suffix;
  std::string args_str;
};

struct ParsedLirTypedCallView {
  std::vector<std::string_view> param_types;
  std::vector<LirTypedCallArgView> args;
};

struct ParsedLirDirectGlobalTypedCallView {
  std::string_view symbol_name;
  ParsedLirTypedCallView typed_call;
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

inline bool lir_call_has_direct_global_callee(std::string_view callee,
                                              std::string_view expected_symbol_name) {
  const auto parsed = parse_lir_direct_global_callee(callee);
  return parsed.has_value() && *parsed == expected_symbol_name;
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
inline void collect_lir_global_symbol_refs_from_text(std::string_view text, Fn&& visit) {
  std::size_t pos = 0;
  while ((pos = text.find('@', pos)) != std::string_view::npos) {
    ++pos;
    if (pos < text.size() && text[pos] == '"') {
      ++pos;
      const std::size_t start = pos;
      while (pos < text.size() && text[pos] != '"') {
        ++pos;
      }
      if (pos > start) {
        visit(text.substr(start - 1, pos - start + 2));
      }
      if (pos < text.size()) {
        ++pos;
      }
      continue;
    }

    const std::size_t start = pos;
    while (pos < text.size() &&
           (std::isalnum(static_cast<unsigned char>(text[pos])) || text[pos] == '_' ||
            text[pos] == '.')) {
      ++pos;
    }
    if (pos > start) {
      visit(text.substr(start, pos - start));
    }
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

inline std::string format_lir_typed_call_arg(std::string_view type,
                                             std::string_view operand) {
  std::string formatted(trim_lir_arg_text(type));
  const auto trimmed_operand = trim_lir_arg_text(operand);
  if (!formatted.empty() && !trimmed_operand.empty()) {
    formatted.push_back(' ');
  }
  formatted.append(trimmed_operand);
  return formatted;
}

inline std::string format_lir_typed_call_args(
    const std::vector<OwnedLirTypedCallArg>& args) {
  std::string formatted;
  for (std::size_t index = 0; index < args.size(); ++index) {
    if (index != 0) {
      formatted += ", ";
    }
    formatted += format_lir_typed_call_arg(args[index].type, args[index].operand);
  }
  return formatted;
}

inline std::vector<OwnedLirTypedCallArg> own_lir_typed_call_args(
    const ParsedLirTypedCallView& parsed) {
  std::vector<OwnedLirTypedCallArg> owned_args;
  owned_args.reserve(parsed.args.size());
  for (const auto& arg : parsed.args) {
    owned_args.push_back(
        {std::string(arg.type), std::string(arg.operand), LirTypeRef(std::string(arg.type))});
  }
  return owned_args;
}

inline ParsedLirTypedCallView borrow_lir_typed_call(
    const std::vector<std::string>& param_types,
    const std::vector<OwnedLirTypedCallArg>& args) {
  ParsedLirTypedCallView borrowed;
  borrowed.param_types.reserve(param_types.size());
  for (const auto& type : param_types) {
    borrowed.param_types.push_back(type);
  }
  borrowed.args.reserve(args.size());
  for (const auto& arg : args) {
    borrowed.args.push_back({arg.type, arg.operand});
  }
  return borrowed;
}

inline std::string format_lir_call_param_types(
    const std::vector<std::string>& param_types) {
  std::string formatted("(");
  for (std::size_t index = 0; index < param_types.size(); ++index) {
    if (index != 0) {
      formatted += ", ";
    }
    formatted += trim_lir_arg_text(param_types[index]);
  }
  formatted.push_back(')');
  return formatted;
}

inline std::string format_lir_call_param_types(
    const std::vector<std::string_view>& param_types) {
  std::string formatted("(");
  for (std::size_t index = 0; index < param_types.size(); ++index) {
    if (index != 0) {
      formatted += ", ";
    }
    formatted += trim_lir_arg_text(param_types[index]);
  }
  formatted.push_back(')');
  return formatted;
}

inline FormattedLirTypedCall format_lir_typed_call(
    const std::vector<OwnedLirTypedCallArg>& args) {
  std::vector<std::string_view> param_types;
  param_types.reserve(args.size());
  for (const auto& arg : args) {
    param_types.push_back(arg.type);
  }
  return {
      format_lir_call_param_types(param_types),
      format_lir_typed_call_args(args),
  };
}

inline FormattedLirTypedCall format_lir_call_fields(
    std::string_view callee_type_suffix,
    const std::vector<OwnedLirTypedCallArg>& args) {
  FormattedLirTypedCall formatted;
  formatted.args_str = format_lir_typed_call_args(args);

  const auto trimmed_suffix = trim_lir_arg_text(callee_type_suffix);
  if (trimmed_suffix.empty()) {
    return formatted;
  }

  const auto parsed_param_types = parse_lir_call_param_types(trimmed_suffix);
  if (!parsed_param_types.has_value()) {
    formatted.callee_type_suffix = std::string(trimmed_suffix);
    return formatted;
  }

  formatted.callee_type_suffix = format_lir_call_param_types(*parsed_param_types);
  return formatted;
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

inline std::optional<ParsedLirTypedCallView> parse_lir_typed_call_or_infer_params(
    std::string_view callee_type_suffix,
    std::string_view args_str) {
  if (const auto parsed = parse_lir_typed_call(callee_type_suffix, args_str);
      parsed.has_value()) {
    return parsed;
  }

  if (!trim_lir_arg_text(callee_type_suffix).empty()) {
    return std::nullopt;
  }

  const auto args = parse_lir_typed_call_args(args_str);
  if (!args.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string_view> param_types;
  param_types.reserve(args->size());
  for (const auto& arg : *args) {
    param_types.push_back(arg.type);
  }
  return ParsedLirTypedCallView{std::move(param_types), *args};
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

inline std::optional<ParsedLirDirectGlobalTypedCallView>
parse_lir_direct_global_typed_call(std::string_view callee,
                                   std::string_view callee_type_suffix,
                                   std::string_view args_str) {
  const auto symbol_name = parse_lir_direct_global_callee(callee);
  if (!symbol_name.has_value()) {
    return std::nullopt;
  }
  const auto typed_call = parse_lir_typed_call(callee_type_suffix, args_str);
  if (!typed_call.has_value()) {
    return std::nullopt;
  }
  return ParsedLirDirectGlobalTypedCallView{*symbol_name, *typed_call};
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

inline void collect_lir_value_names_from_call(std::string_view callee,
                                              std::string_view args_str,
                                              std::vector<std::string>& values) {
  if (is_lir_value_name(callee)) {
    append_unique_lir_value_name(values, std::string(callee));
  } else {
    collect_lir_value_names_from_text(callee, values);
  }
  collect_lir_value_names_from_call_args(args_str, values);
}

template <typename Fn>
inline void collect_lir_global_symbol_refs_from_call_args(std::string_view args_str,
                                                          Fn&& visit) {
  for_each_lir_call_arg(args_str, [&](std::string_view arg) {
    const auto operand = lir_call_arg_operand(arg);
    if (operand.has_value()) {
      const auto parsed_operand = parse_lir_call_callee(*operand);
      if (parsed_operand.has_value() &&
          parsed_operand->kind != LirCallCalleeKind::Indirect) {
        visit(parsed_operand->symbol_name);
        return;
      }
    }
    collect_lir_global_symbol_refs_from_text(arg, visit);
  });
}

template <typename Fn>
inline void collect_lir_global_symbol_refs_from_call(std::string_view callee,
                                                     std::string_view args_str,
                                                     Fn&& visit) {
  const auto parsed_callee = parse_lir_call_callee(callee);
  if (parsed_callee.has_value() && parsed_callee->kind != LirCallCalleeKind::Indirect) {
    visit(parsed_callee->symbol_name);
  } else {
    collect_lir_global_symbol_refs_from_text(callee, visit);
  }
  collect_lir_global_symbol_refs_from_call_args(args_str, std::forward<Fn>(visit));
}

template <typename Fn>
inline std::string rewrite_lir_call_args(std::string_view args_str, Fn&& rewrite_operand) {
  std::vector<std::string> rewritten_args;
  bool parse_failed = false;
  bool any_rewritten = false;

  for_each_lir_call_arg(args_str, [&](std::string_view arg) {
    const auto operand = lir_call_arg_operand(arg);
    if (!operand.has_value()) {
      parse_failed = true;
      return;
    }

    auto rewritten_operand = rewrite_operand(*operand);
    if (!rewritten_operand.has_value() || *rewritten_operand == *operand) {
      rewritten_args.emplace_back(trim_lir_arg_text(arg));
      return;
    }

    const auto operand_offset = static_cast<std::size_t>(operand->data() - arg.data());
    std::string rewritten(trim_lir_arg_text(arg.substr(0, operand_offset)));
    if (!rewritten.empty()) {
      rewritten.push_back(' ');
    }
    rewritten += *rewritten_operand;
    rewritten_args.push_back(std::move(rewritten));
    any_rewritten = true;
  });

  if (parse_failed || !any_rewritten) {
    return std::string(args_str);
  }

  std::string rewritten;
  for (std::size_t index = 0; index < rewritten_args.size(); ++index) {
    if (index != 0) {
      rewritten += ", ";
    }
    rewritten += rewritten_args[index];
  }
  return rewritten;
}

template <typename Fn>
inline void rewrite_lir_call_operands(std::string& callee,
                                      std::string& args_str,
                                      Fn&& rewrite_operand) {
  if (const auto rewritten_callee = rewrite_operand(callee);
      rewritten_callee.has_value()) {
    callee = std::string(*rewritten_callee);
  }
  args_str = rewrite_lir_call_args(args_str, std::forward<Fn>(rewrite_operand));
}

inline std::string format_lir_call_site(std::string_view callee,
                                        std::string_view callee_type_suffix,
                                        std::string_view args_str) {
  std::string formatted;
  const auto trimmed_callee = trim_lir_arg_text(callee);

  if (const auto parsed = parse_lir_typed_call(callee_type_suffix, args_str);
      parsed.has_value()) {
    const auto typed_call = format_lir_typed_call(own_lir_typed_call_args(*parsed));
    if (!typed_call.callee_type_suffix.empty()) {
      formatted += typed_call.callee_type_suffix;
      formatted.push_back(' ');
    }
    formatted += trimmed_callee;
    formatted.push_back('(');
    formatted += typed_call.args_str;
    formatted.push_back(')');
    return formatted;
  }

  const auto trimmed_suffix = trim_lir_arg_text(callee_type_suffix);
  if (!trimmed_suffix.empty()) {
    formatted += trimmed_suffix;
    formatted.push_back(' ');
  }
  formatted += trimmed_callee;
  formatted.push_back('(');

  if (const auto parsed_args = parse_lir_typed_call_args(args_str);
      parsed_args.has_value()) {
    formatted += format_lir_typed_call_args(own_lir_typed_call_args(
        ParsedLirTypedCallView{{}, *parsed_args}));
  } else {
    formatted += args_str;
  }

  formatted.push_back(')');
  return formatted;
}

}  // namespace c4c::codegen::lir
