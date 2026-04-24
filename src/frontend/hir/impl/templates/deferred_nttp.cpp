#include "templates.hpp"
#include "consteval.hpp"

#include <climits>
#include <cstring>
#include <functional>

namespace c4c::hir {

namespace {

void assign_template_arg_refs_from_hir_args(
    TypeSpec* ts,
    const std::vector<HirTemplateArg>& args) {
  if (!ts) return;
  ts->tpl_struct_args.data = nullptr;
  ts->tpl_struct_args.size = 0;
  if (args.empty()) return;

  ts->tpl_struct_args.data = new TemplateArgRef[args.size()]();
  ts->tpl_struct_args.size = static_cast<int>(args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    const HirTemplateArg& arg = args[i];
    TemplateArgRef& out = ts->tpl_struct_args.data[i];
    out.kind = arg.is_value ? TemplateArgKind::Value
                            : TemplateArgKind::Type;
    out.type = arg.is_value ? TypeSpec{} : arg.type;
    out.value = arg.is_value ? arg.value : 0;
    const std::string debug_text =
        arg.is_value ? std::to_string(arg.value)
                     : encode_template_type_arg_ref_hir(arg.type);
    out.debug_text =
        debug_text.empty() ? nullptr : ::strdup(debug_text.c_str());
  }
}

bool parse_pack_binding_name(const std::string& key,
                             const std::string& base,
                             int* out_index = nullptr) {
  if (key.size() <= base.size() + 1) return false;
  if (key.compare(0, base.size(), base) != 0) return false;
  if (key[base.size()] != '#') return false;
  int index = 0;
  try {
    index = std::stoi(key.substr(base.size() + 1));
  } catch (...) {
    return false;
  }
  if (index < 0) return false;
  if (out_index) *out_index = index;
  return true;
}

long long count_pack_bindings_for_name(
    const std::unordered_map<std::string, TypeSpec>& bindings,
    const std::unordered_map<std::string, long long>& nttp_bindings,
    const std::string& base) {
  int max_index = -1;
  for (const auto& [key, _] : bindings) {
    int pack_index = 0;
    if (parse_pack_binding_name(key, base, &pack_index))
      max_index = std::max(max_index, pack_index);
  }
  for (const auto& [key, _] : nttp_bindings) {
    int pack_index = 0;
    if (parse_pack_binding_name(key, base, &pack_index))
      max_index = std::max(max_index, pack_index);
  }
  return max_index + 1;
}

struct DeferredNttpExprCursor {
  const std::string& input;
  size_t pos = 0;

  void skip_ws() {
    while (pos < input.size() &&
           std::isspace(static_cast<unsigned char>(input[pos]))) {
      ++pos;
    }
  }

  bool consume(const char* text) {
    skip_ws();
    const size_t len = std::strlen(text);
    if (input.compare(pos, len, text) == 0) {
      pos += len;
      return true;
    }
    return false;
  }

  std::string parse_identifier() {
    skip_ws();
    const size_t start = pos;
    if (pos >= input.size() ||
        !(std::isalpha(static_cast<unsigned char>(input[pos])) ||
          input[pos] == '_')) {
      return {};
    }
    ++pos;
    while (pos < input.size() &&
           (std::isalnum(static_cast<unsigned char>(input[pos])) ||
            input[pos] == '_')) {
      ++pos;
    }
    return input.substr(start, pos - start);
  }

  bool parse_number(long long* out_val) {
    skip_ws();
    const size_t start = pos;
    if (pos < input.size() &&
        std::isdigit(static_cast<unsigned char>(input[pos]))) {
      ++pos;
      while (pos < input.size() &&
             std::isdigit(static_cast<unsigned char>(input[pos]))) {
        ++pos;
      }
      *out_val =
          std::strtoll(input.substr(start, pos - start).c_str(), nullptr, 10);
      return true;
    }
    return false;
  }

  std::string parse_arg_text() {
    skip_ws();
    const size_t start = pos;
    int angle_depth = 0;
    int paren_depth = 0;
    while (pos < input.size()) {
      const char ch = input[pos];
      if (ch == '<') ++angle_depth;
      else if (ch == '>') {
        if (angle_depth == 0) break;
        --angle_depth;
      } else if (ch == '(') {
        ++paren_depth;
      } else if (ch == ')') {
        if (paren_depth > 0) --paren_depth;
      } else if (ch == ',' && angle_depth == 0 && paren_depth == 0) {
        break;
      }
      ++pos;
    }
    return trim_copy(input.substr(start, pos - start));
  }

  bool at_end() {
    skip_ws();
    return pos == input.size();
  }
};

struct DeferredNttpExprEnv {
  std::unordered_map<std::string, TypeSpec> type_lookup;
  std::unordered_map<std::string, long long> nttp_lookup;

  static DeferredNttpExprEnv from_bindings(
      const std::vector<std::pair<std::string, TypeSpec>>& type_bindings_vec,
      const std::vector<std::pair<std::string, long long>>& nttp_bindings_vec) {
    DeferredNttpExprEnv env;
    for (const auto& [name, ts_val] : type_bindings_vec) {
      env.type_lookup[name] = ts_val;
    }
    for (const auto& [name, val] : nttp_bindings_vec) {
      env.nttp_lookup[name] = val;
    }
    return env;
  }

  bool resolve_arg_text(const std::string& text, HirTemplateArg* out_arg) const {
    if (!out_arg || text.empty()) return false;
    if (text.size() > 1 && text[0] == '@') {
      const size_t colon = text.find(':', 1);
      if (colon == std::string::npos) return false;
      const std::string inner_origin = text.substr(1, colon - 1);
      const size_t member_sep = text.find('$', colon + 1);
      const std::string inner_args =
          member_sep == std::string::npos
              ? text.substr(colon + 1)
              : text.substr(colon + 1, member_sep - (colon + 1));
      std::function<bool(const std::string&, HirTemplateArg*)> parse_nested_arg;
      parse_nested_arg = [&](const std::string& ref,
                             HirTemplateArg* nested_out) -> bool {
        if (!nested_out || ref.empty()) return false;
        auto tit_inner = type_lookup.find(ref);
        if (tit_inner != type_lookup.end()) {
          nested_out->is_value = false;
          nested_out->type = tit_inner->second;
          return true;
        }
        auto nit_inner = nttp_lookup.find(ref);
        if (nit_inner != nttp_lookup.end()) {
          nested_out->is_value = true;
          nested_out->value = nit_inner->second;
          return true;
        }
        if (ref == "true" || ref == "false") {
          nested_out->is_value = true;
          nested_out->value = (ref == "true") ? 1 : 0;
          return true;
        }
        char* nested_end = nullptr;
        const long long parsed_val = std::strtoll(ref.c_str(), &nested_end, 10);
        if (nested_end && *nested_end == '\0') {
          nested_out->is_value = true;
          nested_out->value = parsed_val;
          return true;
        }
        if (ref.size() > 1 && ref[0] == '@') {
          return resolve_arg_text(ref, nested_out);
        }
        TypeSpec nested_builtin{};
        if (parse_builtin_typespec_text(ref, &nested_builtin)) {
          nested_out->is_value = false;
          nested_out->type = nested_builtin;
          return true;
        }
        return false;
      };

      TypeSpec nested_ts{};
      nested_ts.base = TB_STRUCT;
      nested_ts.array_size = -1;
      nested_ts.inner_rank = -1;
      nested_ts.tpl_struct_origin = ::strdup(inner_origin.c_str());
      if (!inner_args.empty()) {
        std::vector<HirTemplateArg> nested_args;
        const auto parts = split_deferred_template_arg_refs(inner_args);
        nested_args.reserve(parts.size());
        for (const auto& part : parts) {
          HirTemplateArg nested_arg{};
          if (!parse_nested_arg(part, &nested_arg)) return false;
          nested_args.push_back(nested_arg);
        }
        assign_template_arg_refs_from_hir_args(&nested_ts, nested_args);
      }
      if (member_sep != std::string::npos && member_sep + 1 < text.size()) {
        nested_ts.deferred_member_type_name =
            ::strdup(text.substr(member_sep + 1).c_str());
      }
      out_arg->is_value = false;
      out_arg->type = nested_ts;
      return true;
    }
    auto tit = type_lookup.find(text);
    if (tit != type_lookup.end()) {
      out_arg->is_value = false;
      out_arg->type = tit->second;
      return true;
    }
    auto nit = nttp_lookup.find(text);
    if (nit != nttp_lookup.end()) {
      out_arg->is_value = true;
      out_arg->value = nit->second;
      return true;
    }
    if (text == "true" || text == "false") {
      out_arg->is_value = true;
      out_arg->value = (text == "true") ? 1 : 0;
      return true;
    }
    char* end = nullptr;
    long long parsed = std::strtoll(text.c_str(), &end, 10);
    if (end && *end == '\0') {
      out_arg->is_value = true;
      out_arg->value = parsed;
      return true;
    }
    TypeSpec builtin{};
    if (parse_builtin_typespec_text(text, &builtin)) {
      out_arg->is_value = false;
      out_arg->type = builtin;
      return true;
    }
    return false;
  }

  bool lookup_bound_value(const std::string& ident, long long* out_val) const {
    auto nit = nttp_lookup.find(ident);
    if (nit != nttp_lookup.end()) {
      *out_val = nit->second;
      return true;
    }
    if (ident == "true") {
      *out_val = 1;
      return true;
    }
    if (ident == "false") {
      *out_val = 0;
      return true;
    }
    return false;
  }

  bool lookup_cast_type(const std::string& ident, TypeSpec* out_ts) const {
    auto tit = type_lookup.find(ident);
    if (tit != type_lookup.end()) {
      *out_ts = tit->second;
      return true;
    }
    return parse_builtin_typespec_text(ident, out_ts);
  }

  long long count_pack_bindings(const std::string& pack_name) const {
    return count_pack_bindings_for_name(type_lookup, nttp_lookup, pack_name);
  }
};

struct DeferredNttpTemplateLookup {
  const std::unordered_map<std::string, const Node*>& template_defs;
  const std::unordered_map<std::string, std::vector<const Node*>>&
      specializations;
  const std::unordered_map<std::string, const Node*>& struct_defs;

  bool eval_template_static_member_lookup(
      const std::string& tpl_name,
      const std::vector<HirTemplateArg>& actual_args,
      const std::string& member_name,
      long long* out_val) const {
    auto tpl_it = template_defs.find(tpl_name);
    if (tpl_it == template_defs.end()) return false;
    const Node* ref_primary = tpl_it->second;

    TemplateStructEnv ref_env;
    ref_env.primary_def = ref_primary;
    if (ref_primary && ref_primary->name) {
      auto it = specializations.find(ref_primary->name);
      if (it != specializations.end()) ref_env.specialization_patterns = &it->second;
    }

    SelectedTemplateStructPattern ref_selection =
        select_template_struct_pattern_hir(actual_args, ref_env);
    if (!ref_selection.selected_pattern) return false;

    std::string ref_mangled;
    if (ref_selection.selected_pattern != ref_primary &&
        ref_selection.selected_pattern->n_template_params == 0 &&
        ref_selection.selected_pattern->name &&
        ref_selection.selected_pattern->name[0]) {
      ref_mangled = ref_selection.selected_pattern->name;
    } else {
      const std::string ref_family =
          (ref_selection.selected_pattern &&
           ref_selection.selected_pattern->template_origin_name &&
           ref_selection.selected_pattern->template_origin_name[0])
              ? ref_selection.selected_pattern->template_origin_name
              : tpl_name;
      ref_mangled = ref_family;
      for (int pi = 0; pi < ref_primary->n_template_params; ++pi) {
        ref_mangled += "_";
        ref_mangled += ref_primary->template_param_names[pi];
        ref_mangled += "_";
        if (pi < static_cast<int>(actual_args.size()) && actual_args[pi].is_value) {
          ref_mangled += std::to_string(actual_args[pi].value);
        } else if (pi < static_cast<int>(actual_args.size()) &&
                   !actual_args[pi].is_value) {
          ref_mangled += type_suffix_for_mangling(actual_args[pi].type);
        }
      }
    }

    auto concrete_it = struct_defs.find(ref_mangled);
    if (concrete_it != struct_defs.end()) {
      if (eval_struct_static_member_value_hir(
              concrete_it->second, struct_defs, member_name, nullptr, out_val)) {
        return true;
      }
    }

    return eval_struct_static_member_value_hir(
        ref_selection.selected_pattern, struct_defs, member_name,
        &ref_selection.nttp_bindings, out_val);
  }
};

struct DeferredNttpExprParser {
  using ParseExprFn = bool (DeferredNttpExprParser::*)(long long*);
  using ApplyBinaryFn = bool (*)(long long*, long long);

  struct BinaryOpSpec {
    const char* token;
    ApplyBinaryFn apply;
  };

  DeferredNttpExprCursor cursor;
  const DeferredNttpTemplateLookup& template_lookup;
  const DeferredNttpExprEnv& env;

  static long long apply_integral_cast(TypeSpec ts, long long v) {
    if (ts.ptr_level != 0) return v;
    int bits = 0;
    switch (ts.base) {
      case TB_BOOL: bits = 1; break;
      case TB_CHAR:
      case TB_UCHAR:
      case TB_SCHAR: bits = 8; break;
      case TB_SHORT:
      case TB_USHORT: bits = 16; break;
      case TB_INT:
      case TB_UINT:
      case TB_ENUM: bits = 32; break;
      default: break;
    }
    if (bits <= 0 || bits >= 64) return v;
    long long mask = (1LL << bits) - 1;
    v &= mask;
    if (!is_unsigned_base(ts.base) && ts.base != TB_BOOL && (v >> (bits - 1)))
      v |= ~mask;
    return v;
  }

  static bool apply_mul_op(long long* lhs, long long rhs) {
    *lhs *= rhs;
    return true;
  }

  static bool apply_div_op(long long* lhs, long long rhs) {
    if (rhs == 0) return false;
    *lhs /= rhs;
    return true;
  }

  static bool apply_add_op(long long* lhs, long long rhs) {
    *lhs += rhs;
    return true;
  }

  static bool apply_sub_op(long long* lhs, long long rhs) {
    *lhs -= rhs;
    return true;
  }

  static bool apply_le_op(long long* lhs, long long rhs) {
    *lhs = (*lhs <= rhs) ? 1 : 0;
    return true;
  }

  static bool apply_ge_op(long long* lhs, long long rhs) {
    *lhs = (*lhs >= rhs) ? 1 : 0;
    return true;
  }

  static bool apply_lt_op(long long* lhs, long long rhs) {
    *lhs = (*lhs < rhs) ? 1 : 0;
    return true;
  }

  static bool apply_gt_op(long long* lhs, long long rhs) {
    *lhs = (*lhs > rhs) ? 1 : 0;
    return true;
  }

  static bool apply_eq_op(long long* lhs, long long rhs) {
    *lhs = (*lhs == rhs) ? 1 : 0;
    return true;
  }

  static bool apply_ne_op(long long* lhs, long long rhs) {
    *lhs = (*lhs != rhs) ? 1 : 0;
    return true;
  }

  static bool apply_and_op(long long* lhs, long long rhs) {
    *lhs = (*lhs && rhs) ? 1 : 0;
    return true;
  }

  static bool apply_or_op(long long* lhs, long long rhs) {
    *lhs = (*lhs || rhs) ? 1 : 0;
    return true;
  }

  bool parse_left_associative(
      long long* out_val, ParseExprFn operand_parser,
      std::initializer_list<BinaryOpSpec> ops) {
    if (!(this->*operand_parser)(out_val)) return false;
    while (true) {
      cursor.skip_ws();
      const BinaryOpSpec* matched = nullptr;
      for (const BinaryOpSpec& spec : ops) {
        if (cursor.consume(spec.token)) {
          matched = &spec;
          break;
        }
      }
      if (!matched) break;
      long long rhs = 0;
      if (!(this->*operand_parser)(&rhs)) return false;
      if (!matched->apply(out_val, rhs)) return false;
    }
    return true;
  }

  bool resolve_arg(const std::string& text, HirTemplateArg* out_arg) {
    return env.resolve_arg_text(text, out_arg);
  }

  bool eval_member_lookup(long long* out_val) {
    const std::string tpl_name = cursor.parse_identifier();
    if (tpl_name.empty() || !cursor.consume("<")) return false;

    std::vector<HirTemplateArg> actual_args;
    if (!parse_template_arg_list(&actual_args)) return false;

    std::string member_name;
    if (!parse_template_member_name(&member_name)) return false;
    return template_lookup.eval_template_static_member_lookup(
        tpl_name, actual_args, member_name, out_val);
  }

  bool parse_template_arg_list(std::vector<HirTemplateArg>* out_args) {
    if (!out_args) return false;
    cursor.skip_ws();
    if (!cursor.consume(">")) {
      while (true) {
        HirTemplateArg arg;
        if (!resolve_arg(cursor.parse_arg_text(), &arg)) return false;
        out_args->push_back(arg);
        cursor.skip_ws();
        if (cursor.consume(">")) break;
        if (!cursor.consume(",")) return false;
      }
    }
    return true;
  }

  bool parse_template_member_name(std::string* out_member_name) {
    if (!out_member_name) return false;
    if (!cursor.consume("::")) return false;
    *out_member_name = cursor.parse_identifier();
    return !out_member_name->empty();
  }

  bool parse_pack_sizeof(long long* out_val) {
    if (!cursor.consume("sizeof")) return false;
    cursor.skip_ws();
    if (!cursor.consume("...")) return false;
    if (!cursor.consume("(")) return false;
    const std::string pack_name = cursor.parse_identifier();
    if (pack_name.empty()) return false;
    if (!cursor.consume(")")) return false;
    *out_val = env.count_pack_bindings(pack_name);
    return true;
  }

  bool parse_parenthesized_expr(long long* out_val) {
    if (!cursor.consume("(")) return false;
    if (!parse_or(out_val)) return false;
    return cursor.consume(")");
  }

  bool parse_cast_expr(long long* out_val) {
    size_t saved = cursor.pos;
    std::string ident = cursor.parse_identifier();
    if (ident.empty()) return false;
    cursor.skip_ws();
    if (!cursor.consume("(")) {
      cursor.pos = saved;
      return false;
    }

    TypeSpec cast_ts{};
    cast_ts.array_size = -1;
    cast_ts.inner_rank = -1;
    if (!env.lookup_cast_type(ident, &cast_ts)) {
      cursor.pos = saved;
      return false;
    }

    long long inner = 0;
    if (!parse_or(&inner)) return false;
    if (!cursor.consume(")")) return false;
    *out_val = apply_integral_cast(cast_ts, inner);
    return true;
  }

  bool parse_parenthesized_or_cast(long long* out_val) {
    if (parse_parenthesized_expr(out_val)) return true;
    return parse_cast_expr(out_val);
  }

  bool parse_bound_identifier(long long* out_val) {
    size_t saved = cursor.pos;
    std::string ident = cursor.parse_identifier();
    if (ident.empty()) return false;
    if (env.lookup_bound_value(ident, out_val)) return true;
    cursor.pos = saved;
    return false;
  }

  bool parse_primary(long long* out_val) {
    cursor.skip_ws();
    if (parse_pack_sizeof(out_val)) return true;
    if (parse_parenthesized_or_cast(out_val)) return true;
    if (parse_numeric_literal(out_val)) return true;
    if (parse_bound_identifier(out_val)) return true;
    return eval_member_lookup(out_val);
  }

  bool parse_numeric_literal(long long* out_val) {
    return cursor.parse_number(out_val);
  }

  bool parse_unary(long long* out_val) {
    cursor.skip_ws();
    if (cursor.consume("!")) {
      long long inner = 0;
      if (!parse_unary(&inner)) return false;
      *out_val = inner ? 0 : 1;
      return true;
    }
    if (cursor.consume("-")) {
      long long inner = 0;
      if (!parse_unary(&inner)) return false;
      *out_val = -inner;
      return true;
    }
    if (cursor.consume("+")) return parse_unary(out_val);
    return parse_primary(out_val);
  }

  bool parse_mul(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_unary,
                                  {{"*", &DeferredNttpExprParser::apply_mul_op},
                                   {"/", &DeferredNttpExprParser::apply_div_op}});
  }

  bool parse_add(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_mul,
                                  {{"+", &DeferredNttpExprParser::apply_add_op},
                                   {"-", &DeferredNttpExprParser::apply_sub_op}});
  }

  bool parse_rel(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_add,
                                  {{"<=", &DeferredNttpExprParser::apply_le_op},
                                   {">=", &DeferredNttpExprParser::apply_ge_op},
                                   {"<", &DeferredNttpExprParser::apply_lt_op},
                                   {">", &DeferredNttpExprParser::apply_gt_op}});
  }

  bool parse_eq(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_rel,
                                  {{"==", &DeferredNttpExprParser::apply_eq_op},
                                   {"!=", &DeferredNttpExprParser::apply_ne_op}});
  }

  bool parse_and(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_eq,
                                  {{"&&", &DeferredNttpExprParser::apply_and_op}});
  }

  bool parse_or(long long* out_val) {
    return parse_left_associative(out_val, &DeferredNttpExprParser::parse_and,
                                  {{"||", &DeferredNttpExprParser::apply_or_op}});
  }
};

}  // namespace

bool Lowerer::eval_deferred_nttp_expr_hir(
    const Node* owner_tpl, int param_idx,
    const std::vector<std::pair<std::string, TypeSpec>>& type_bindings_vec,
    const std::vector<std::pair<std::string, long long>>& nttp_bindings_vec,
    const std::string* expr_override,
    long long* out) {
  if (!owner_tpl || param_idx < 0 || param_idx >= owner_tpl->n_template_params) {
    return false;
  }

  std::string expr;
  if (expr_override) {
    expr = *expr_override;
  } else {
    if (!owner_tpl->template_param_default_exprs ||
        !owner_tpl->template_param_default_exprs[param_idx]) {
      return false;
    }
    expr = owner_tpl->template_param_default_exprs[param_idx];
  }
  if (expr.empty()) return false;

  DeferredNttpExprEnv env =
      DeferredNttpExprEnv::from_bindings(type_bindings_vec, nttp_bindings_vec);
  DeferredNttpTemplateLookup template_lookup{template_struct_defs_,
                                             template_struct_specializations_,
                                             struct_def_nodes_};
  DeferredNttpExprParser parser{{expr, 0}, template_lookup, env};
  long long value = 0;
  if (!parser.parse_or(&value)) return false;
  if (!parser.cursor.at_end()) return false;
  *out = value;
  return true;
}

}  // namespace c4c::hir
