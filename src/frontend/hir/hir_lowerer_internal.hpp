#pragma once

#include "ast_to_hir.hpp"
#include "type_utils.hpp"
#include "../parser/parser_internal.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::hir {

inline std::string trim_copy(std::string s) {
  size_t start = 0;
  while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
  size_t end = s.size();
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
  return s.substr(start, end - start);
}

inline bool parse_builtin_typespec_text(const std::string& text, TypeSpec* out) {
  if (!out) return false;
  std::istringstream iss(text);
  std::string tok;
  TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = TB_INT;
  bool saw_base = false;
  bool is_unsigned = false;
  bool is_signed = false;
  int long_count = 0;
  while (iss >> tok) {
    if (tok == "const") ts.is_const = true;
    else if (tok == "volatile") ts.is_volatile = true;
    else if (tok == "unsigned") is_unsigned = true;
    else if (tok == "signed") is_signed = true;
    else if (tok == "uint") { ts.base = TB_UINT; saw_base = true; }
    else if (tok == "schar") { ts.base = TB_SCHAR; saw_base = true; }
    else if (tok == "uchar") { ts.base = TB_UCHAR; saw_base = true; }
    else if (tok == "ushort") { ts.base = TB_USHORT; saw_base = true; }
    else if (tok == "ulong") { ts.base = TB_ULONG; saw_base = true; }
    else if (tok == "llong") { ts.base = TB_LONGLONG; saw_base = true; }
    else if (tok == "ullong") { ts.base = TB_ULONGLONG; saw_base = true; }
    else if (tok == "ldouble") { ts.base = TB_LONGDOUBLE; saw_base = true; }
    else if (tok == "i128") { ts.base = TB_INT128; saw_base = true; }
    else if (tok == "u128") { ts.base = TB_UINT128; saw_base = true; }
    else if (tok == "void") { ts.base = TB_VOID; saw_base = true; }
    else if (tok == "bool") { ts.base = TB_BOOL; saw_base = true; }
    else if (tok == "char") {
      ts.base = is_unsigned ? TB_UCHAR : (is_signed ? TB_SCHAR : TB_CHAR);
      saw_base = true;
    } else if (tok == "short") {
      ts.base = is_unsigned ? TB_USHORT : TB_SHORT;
      saw_base = true;
    } else if (tok == "int") {
      if (long_count >= 2) ts.base = is_unsigned ? TB_ULONGLONG : TB_LONGLONG;
      else if (long_count == 1) ts.base = is_unsigned ? TB_ULONG : TB_LONG;
      else ts.base = is_unsigned ? TB_UINT : TB_INT;
      saw_base = true;
    } else if (tok == "long") {
      ++long_count;
      ts.base = (long_count >= 2)
                    ? (is_unsigned ? TB_ULONGLONG : TB_LONGLONG)
                    : (is_unsigned ? TB_ULONG : TB_LONG);
      saw_base = true;
    } else if (tok == "*") {
      ++ts.ptr_level;
    } else {
      return false;
    }
  }
  if (!saw_base && ts.ptr_level == 0) return false;
  *out = ts;
  return true;
}
SelectedTemplateStructPattern select_template_struct_pattern_hir(
    const std::vector<HirTemplateArg>& actual_args,
    const TemplateStructEnv& env);
bool eval_struct_static_member_value_hir(
    const Node* sdef,
    const std::unordered_map<std::string, const Node*>& struct_defs,
    const std::string& member_name,
    const NttpBindings* nttp_bindings,
    long long* out);

class Lowerer {
 public:
  const sema::ResolvedTypeTable* resolved_types_ = nullptr;

  /// Engine-owned compile-time state, shared with the pipeline.
  std::shared_ptr<CompileTimeState> ct_state() const;

  void seed_pending_template_type(const TypeSpec& ts,
                                  const TypeBindings& tpl_bindings,
                                  const NttpBindings& nttp_bindings,
                                  const Node* span_node,
                                  PendingTemplateTypeKind kind,
                                  const std::string& context_name = {});

  const Node* canonical_template_struct_primary(
      const TypeSpec& ts,
      const Node* primary_tpl = nullptr) const;

  void resolve_pending_tpl_struct_if_needed(
      TypeSpec& ts,
      const TypeBindings& tpl_bindings,
      const NttpBindings& nttp_bindings,
      const Node* primary_tpl = nullptr);

  std::string format_deferred_template_type_diagnostic(
      const PendingTemplateTypeWorkItem& work_item,
      const char* prefix,
      const char* detail = nullptr) const;

  DeferredTemplateTypeResult blocked_deferred_template_type(
      const PendingTemplateTypeWorkItem& work_item,
      const char* detail,
      bool spawned_new_work = false) const;

  DeferredTemplateTypeResult terminal_deferred_template_type(
      const PendingTemplateTypeWorkItem& work_item,
      const char* detail) const;

  const Node* require_pending_template_type_primary(
      const TypeSpec& ts,
      const PendingTemplateTypeWorkItem& work_item,
      const char* missing_detail,
      DeferredTemplateTypeResult* out_result) const;

  bool resolve_pending_template_owner_if_ready(
      TypeSpec& owner_ts,
      const PendingTemplateTypeWorkItem& work_item,
      const Node* primary_tpl,
      bool spawned_owner_work,
      const char* pending_detail,
      DeferredTemplateTypeResult* out_result);

  bool spawn_pending_template_owner_work(
      const PendingTemplateTypeWorkItem& work_item,
      const TypeSpec& owner_ts);

  bool ensure_pending_template_owner_ready(
      TypeSpec& owner_ts,
      const PendingTemplateTypeWorkItem& work_item,
      bool spawn_owner_work,
      const char* missing_detail,
      const char* pending_detail,
      DeferredTemplateTypeResult* out_result);

  DeferredTemplateTypeResult resolve_deferred_member_typedef_type(
      const PendingTemplateTypeWorkItem& work_item);

  void seed_template_type_dependency_if_needed(
      const TypeSpec& ts,
      const TypeBindings& tpl_bindings,
      const NttpBindings& nttp_bindings,
      PendingTemplateTypeKind kind,
      const std::string& context_name,
      const Node* span_node = nullptr);

  void seed_and_resolve_pending_template_type_if_needed(
      TypeSpec& ts,
      const TypeBindings& tpl_bindings,
      const NttpBindings& nttp_bindings,
      const Node* span_node,
      PendingTemplateTypeKind kind,
      const std::string& context_name,
      const Node* primary_tpl = nullptr);

  bool resolve_struct_member_typedef_if_ready(TypeSpec* ts);

  std::vector<const Node*> flatten_program_items(const Node* root) const;

  void collect_weak_symbol_names(const std::vector<const Node*>& items);

  void collect_enum_def(const Node* ed);

  void collect_initial_type_definitions(const std::vector<const Node*>& items);

  void collect_consteval_function_definitions(const std::vector<const Node*>& items);

  void collect_template_function_definitions(const std::vector<const Node*>& items);

  void collect_function_template_specializations(const std::vector<const Node*>& items);

  void collect_depth0_template_instantiations(const std::vector<const Node*>& items);

  void run_consteval_template_seed_fixpoint(const std::vector<const Node*>& items);

  void finalize_template_seed_realization();

  void populate_hir_template_defs(Module& m);

  void collect_ref_overloaded_free_functions(const std::vector<const Node*>& items);

  void attach_out_of_class_struct_method_defs(const std::vector<const Node*>& items,
                                              Module& m);

  void lower_non_method_functions_and_globals(const std::vector<const Node*>& items,
                                              Module& m);

  void lower_pending_struct_methods();

  void lower_initial_program(const Node* root, Module& m);

  bool instantiate_deferred_template(const std::string& tpl_name,
                                     const TypeBindings& bindings,
                                     const NttpBindings& nttp_bindings,
                                     const std::string& mangled);

  DeferredTemplateTypeResult instantiate_deferred_template_type(
      const PendingTemplateTypeWorkItem& work_item);

 private:
  struct SwitchCtx {
    BlockId parent_block{};
    std::vector<std::pair<long long, BlockId>> cases;
    std::vector<std::tuple<long long, long long, BlockId>> case_ranges;
    std::optional<BlockId> default_block;
  };

  struct FunctionCtx {
    struct PackParamElem {
      std::string element_name;
      TypeSpec type;
      uint32_t param_index = 0;
    };

    Function* fn = nullptr;
    std::unordered_map<std::string, LocalId> locals;
    std::unordered_map<uint32_t, TypeSpec> local_types;
    std::unordered_map<std::string, FnPtrSig> local_fn_ptr_sigs;
    std::unordered_map<std::string, FnPtrSig> param_fn_ptr_sigs;
    std::unordered_map<std::string, GlobalId> static_globals;
    std::unordered_map<std::string, uint32_t> params;
    BlockId current_block{};
    std::vector<BlockId> break_stack;
    std::vector<BlockId> continue_stack;
    std::unordered_map<std::string, BlockId> label_blocks;
    std::vector<SwitchCtx> switch_stack;
    std::unordered_map<std::string, long long> local_const_bindings;
    TypeBindings tpl_bindings;  // template param → concrete type for enclosing template fn
    NttpBindings nttp_bindings; // non-type template param → constant value
    std::unordered_map<std::string, std::vector<PackParamElem>> pack_params;
    std::string method_struct_tag; // non-empty when lowering a struct method body
    // Destructor tracking: records locals that need destructor calls at scope exit.
    struct DtorLocal {
      LocalId local_id;
      std::string struct_tag;
    };
    std::vector<DtorLocal> dtor_stack;
  };

  static bool is_lvalue_ref_ts(const TypeSpec& ts);

  static std::string pack_binding_name(const std::string& base, int index);

  static bool parse_pack_binding_name(const std::string& key,
                                      const std::string& base,
                                      int* out_index = nullptr);

  static long long count_pack_bindings_for_name(const TypeBindings& bindings,
                                                const NttpBindings& nttp_bindings,
                                                const std::string& base);

  static bool is_any_ref_ts(const TypeSpec& ts);

  static TypeSpec reference_storage_ts(TypeSpec ts);

  static TypeSpec reference_value_ts(TypeSpec ts);

  // Resolve TB_TYPEDEF to TB_STRUCT/TB_UNION when the tag matches a known
  // struct definition.  Handles the injected-class-name case where the parser
  // cannot fully resolve the typedef because the struct is still incomplete
  // during body parsing.
  void resolve_typedef_to_struct(TypeSpec& ts) const;

  FunctionId next_fn_id();
  GlobalId next_global_id();
  LocalId next_local_id();
  BlockId next_block_id();
  ExprId next_expr_id();

  static bool contains_stmt_expr(const Node* n);

  QualType qtype_from(const TypeSpec& t, ValueCategory c = ValueCategory::RValue);

  std::optional<FnPtrSig> fn_ptr_sig_from_decl_node(const Node* n);

  std::optional<TypeSpec> infer_call_result_type_from_callee(
      FunctionCtx* ctx, const Node* callee);

  std::optional<TypeSpec> storage_type_for_declref(FunctionCtx* ctx,
                                                   const DeclRef& r);

  TypeSpec infer_generic_ctrl_type(FunctionCtx* ctx, const Node* n);

  Block& ensure_block(FunctionCtx& ctx, BlockId id);

  BlockId create_block(FunctionCtx& ctx);

  void append_stmt(FunctionCtx& ctx, Stmt stmt);

  const Node* find_struct_static_member_decl(const std::string& tag,
                                             const std::string& member) const;

  std::optional<long long> find_struct_static_member_const_value(
      const std::string& tag, const std::string& member) const;

  long long eval_const_int_with_nttp_bindings(
      const Node* n, const NttpBindings& nttp_bindings) const;

  std::optional<std::string> find_struct_method_mangled(
      const std::string& tag,
      const std::string& method,
      bool is_const_obj) const;

  std::optional<TypeSpec> find_struct_method_return_type(
      const std::string& tag,
      const std::string& method,
      bool is_const_obj) const;

  ExprId append_expr(const Node* src,
                     ExprPayload payload,
                     const TypeSpec& ts,
                     ValueCategory c = ValueCategory::RValue);

  bool eval_deferred_nttp_expr_hir(
      const Node* owner_tpl, int param_idx,
      const std::vector<std::pair<std::string, TypeSpec>>& type_bindings_vec,
      const std::vector<std::pair<std::string, long long>>& nttp_bindings_vec,
      const std::string* expr_override,
      long long* out);

  static bool template_struct_has_pack_params(const Node* primary_tpl);

  ResolvedTemplateArgs materialize_template_args(
      const Node* primary_tpl,
      const std::vector<std::string>& arg_refs,
      const TypeBindings& tpl_bindings,
      const NttpBindings& nttp_bindings);

  void lower_struct_def(const Node* sd);

  bool resolve_struct_member_typedef_hir(const std::string& tag,
                                         const std::string& member,
                                         TypeSpec* out);

  // Resolve a pending template struct type using concrete template bindings.
  // ---------------------------------------------------------------------------
  // Helper 1: Evaluate a deferred NTTP default expression.
  // ---------------------------------------------------------------------------
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
      if (!is_unsigned_base(ts.base) && ts.base != TB_BOOL &&
          (v >> (bits - 1)))
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

  // ---------------------------------------------------------------------------
  // Helper 2: Materialize explicit and default template args into concrete
  // args, resolving type bindings and NTTP bindings along the way.
  // ---------------------------------------------------------------------------
  // ---------------------------------------------------------------------------
  // Helper 3: Prepare identity bindings and the dedup key for a concrete
  // template-struct instance.
  // ---------------------------------------------------------------------------
  PreparedTemplateStructInstance prepare_template_struct_instance(
      const Node* primary_tpl,
      const char* origin,
      const ResolvedTemplateArgs& resolved) {
    PreparedTemplateStructInstance prepared;
    std::vector<std::string> primary_param_order;
    primary_param_order.reserve(primary_tpl->n_template_params);
    for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
      const char* param_name = primary_tpl->template_param_names[pi];
      if (!param_name) continue;
      const bool is_pack =
          primary_tpl->template_param_is_pack &&
          primary_tpl->template_param_is_pack[pi];
      const bool is_nttp =
          primary_tpl->template_param_is_nttp &&
          primary_tpl->template_param_is_nttp[pi];
      if (is_pack) {
        std::vector<std::pair<int, std::string>> pack_names;
        if (is_nttp) {
          for (const auto& [name, _] : resolved.nttp_bindings) {
            int pack_index = 0;
            if (parse_pack_binding_name(name, param_name, &pack_index))
              pack_names.push_back({pack_index, name});
          }
        } else {
          for (const auto& [name, _] : resolved.type_bindings) {
            int pack_index = 0;
            if (parse_pack_binding_name(name, param_name, &pack_index))
              pack_names.push_back({pack_index, name});
          }
        }
        std::sort(pack_names.begin(), pack_names.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& [_, pack_name] : pack_names) {
          primary_param_order.push_back(pack_name);
          if (is_nttp) {
            for (const auto& [name, value] : resolved.nttp_bindings) {
              if (name == pack_name) prepared.nttp_bindings[name] = value;
            }
          } else {
            for (const auto& [name, type] : resolved.type_bindings) {
              if (name == pack_name) prepared.type_bindings[name] = type;
            }
          }
        }
        continue;
      }
      primary_param_order.push_back(param_name);
      if (is_nttp) {
        for (const auto& [name, value] : resolved.nttp_bindings) {
          if (name == param_name) prepared.nttp_bindings[name] = value;
        }
      } else {
        for (const auto& [name, type] : resolved.type_bindings) {
          if (name == param_name) prepared.type_bindings[name] = type;
        }
      }
    }

    const std::string primary_name =
        primary_tpl->name ? primary_tpl->name
                          : std::string(origin ? origin : "");
    SpecializationKey instance_spec_key = prepared.nttp_bindings.empty()
        ? make_specialization_key(primary_name, primary_param_order,
                                  prepared.type_bindings)
        : make_specialization_key(primary_name, primary_param_order,
                                  prepared.type_bindings,
                                  prepared.nttp_bindings);
    prepared.instance_key = TemplateStructInstanceKey{primary_tpl, instance_spec_key};
    return prepared;
  }

  // ---------------------------------------------------------------------------
  // Helper 4: Build the mangled/concrete name for a template struct instance.
  // ---------------------------------------------------------------------------
  static std::string build_template_mangled_name(
      const Node* primary_tpl,
      const Node* tpl_def,
      const char* origin,
      const ResolvedTemplateArgs& resolved) {
    const std::string family_name =
        tpl_def->template_origin_name ? tpl_def->template_origin_name : std::string(origin);
    std::string mangled(family_name);
    auto append_type_suffix = [&](const TypeSpec& pts) {
      switch (pts.base) {
        case TB_INT: mangled += "int"; break;
        case TB_UINT: mangled += "uint"; break;
        case TB_CHAR: mangled += "char"; break;
        case TB_SCHAR: mangled += "schar"; break;
        case TB_UCHAR: mangled += "uchar"; break;
        case TB_SHORT: mangled += "short"; break;
        case TB_USHORT: mangled += "ushort"; break;
        case TB_LONG: mangled += "long"; break;
        case TB_ULONG: mangled += "ulong"; break;
        case TB_LONGLONG: mangled += "llong"; break;
        case TB_ULONGLONG: mangled += "ullong"; break;
        case TB_FLOAT: mangled += "float"; break;
        case TB_DOUBLE: mangled += "double"; break;
        case TB_LONGDOUBLE: mangled += "ldouble"; break;
        case TB_VOID: mangled += "void"; break;
        case TB_BOOL: mangled += "bool"; break;
        case TB_INT128: mangled += "i128"; break;
        case TB_UINT128: mangled += "u128"; break;
        case TB_STRUCT:
          mangled += "struct_";
          mangled += pts.tag ? pts.tag : "anon";
          break;
        case TB_UNION:
          mangled += "union_";
          mangled += pts.tag ? pts.tag : "anon";
          break;
        case TB_ENUM:
          mangled += "enum_";
          mangled += pts.tag ? pts.tag : "anon";
          break;
        case TB_TYPEDEF:
          mangled += pts.tag ? pts.tag : "typedef";
          break;
        default: mangled += "T"; break;
      }
      for (int p = 0; p < pts.ptr_level; ++p) mangled += "_ptr";
      if (pts.is_lvalue_ref) mangled += "_ref";
      if (pts.is_rvalue_ref) mangled += "_rref";
    };
    auto append_pack_entries = [&](const char* param_name, bool is_value_pack) {
      bool wrote_any = false;
      if (is_value_pack) {
        std::vector<std::pair<int, long long>> values;
        for (const auto& [name, value] : resolved.nttp_bindings) {
          int pack_index = 0;
          if (parse_pack_binding_name(name, param_name, &pack_index))
            values.push_back({pack_index, value});
        }
        std::sort(values.begin(), values.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& [idx, value] : values) {
          if (idx > 0) mangled += "_";
          mangled += std::to_string(value);
          wrote_any = true;
        }
      } else {
        std::vector<std::pair<int, TypeSpec>> types;
        for (const auto& [name, type] : resolved.type_bindings) {
          int pack_index = 0;
          if (parse_pack_binding_name(name, param_name, &pack_index))
            types.push_back({pack_index, type});
        }
        std::sort(types.begin(), types.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& [idx, type] : types) {
          if (idx > 0) mangled += "_";
          append_type_suffix(type);
          wrote_any = true;
        }
      }
      if (!wrote_any) mangled += is_value_pack ? "0" : "T";
    };
    for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
      mangled += "_";
      mangled += primary_tpl->template_param_names[pi];
      mangled += "_";
      const bool is_pack =
          primary_tpl->template_param_is_pack &&
          primary_tpl->template_param_is_pack[pi];
      const bool is_nttp =
          primary_tpl->template_param_is_nttp &&
          primary_tpl->template_param_is_nttp[pi];
      if (is_pack) {
        append_pack_entries(primary_tpl->template_param_names[pi], is_nttp);
      } else if (is_nttp) {
        bool wrote_value = false;
        for (const auto& [name, value] : resolved.nttp_bindings) {
          if (name == primary_tpl->template_param_names[pi]) {
            mangled += std::to_string(value);
            wrote_value = true;
            break;
          }
        }
        if (!wrote_value) mangled += "0";
      } else {
        bool wrote_type = false;
        for (const auto& [name, type] : resolved.type_bindings) {
          if (name == primary_tpl->template_param_names[pi]) {
            append_type_suffix(type);
            wrote_type = true;
            break;
          }
        }
        if (!wrote_type) mangled += "T";
      }
    }
    return mangled;
  }

  void apply_template_typedef_bindings(TypeSpec& ts,
                                       const TypeBindings& type_bindings);

  void materialize_template_array_extent(TypeSpec& ts,
                                         const NttpBindings& nttp_bindings);

  void append_instantiated_template_struct_bases(
      HirStructDef& def,
      const Node* tpl_def,
      const TypeBindings& method_tpl_bindings,
      const NttpBindings& method_nttp_bindings);

  void register_instantiated_template_struct_methods(
      const std::string& mangled,
      const Node* tpl_def,
      const TypeBindings& method_tpl_bindings,
      const NttpBindings& method_nttp_bindings);

  void record_instantiated_template_struct_field_metadata(
      const std::string& mangled,
      const Node* orig_f,
      const NttpBindings& selected_nttp_bindings_map);

  std::optional<HirStructField> instantiate_template_struct_field(
      const Node* orig_f,
      const TypeBindings& selected_type_bindings,
      const NttpBindings& selected_nttp_bindings_map,
      const Node* tpl_def,
      int llvm_idx);

  void append_instantiated_template_struct_fields(
      HirStructDef& def,
      const std::string& mangled,
      const Node* tpl_def,
      const TypeBindings& selected_type_bindings,
      const NttpBindings& selected_nttp_bindings_map);

  // ---------------------------------------------------------------------------
  // Helper 5: Instantiate the concrete struct body (fields, bases, methods).
  // ---------------------------------------------------------------------------
  void instantiate_template_struct_body(
      const std::string& mangled,
      const Node* primary_tpl,
      const Node* tpl_def,
      const SelectedTemplateStructPattern& selected_pattern,
      const std::vector<HirTemplateArg>& concrete_args,
      const TemplateStructInstanceKey& instance_key);

  // ---------------------------------------------------------------------------
  // Coordinator: resolve_pending_tpl_struct — now a thin coordinator that
  // delegates to the four helpers above.
  // ---------------------------------------------------------------------------
  void resolve_pending_tpl_struct(TypeSpec& ts,
                                  const Node* primary_tpl,
                                  const TypeBindings& tpl_bindings,
                                  const NttpBindings& nttp_bindings);

  TypeSpec substitute_signature_template_type(
      TypeSpec ts, const TypeBindings* tpl_bindings) const;

  void resolve_signature_template_type_if_needed(
      TypeSpec& ts,
      const TypeBindings* tpl_bindings,
      const NttpBindings* nttp_bindings,
      const Node* span_node,
      const std::string& context_name);

  TypeSpec prepare_callable_return_type(
      TypeSpec ret_ts,
      const TypeBindings* tpl_bindings,
      const NttpBindings* nttp_bindings,
      const Node* span_node,
      const std::string& context_name,
      bool resolve_typedef_struct);

  void append_explicit_callable_param(
      Function& fn,
      FunctionCtx& ctx,
      const Node* param_node,
      const std::string& emitted_name,
      TypeSpec param_ts,
      const TypeBindings* tpl_bindings,
      const NttpBindings* nttp_bindings,
      const std::string& context_name,
      bool resolve_typedef_struct);

  void append_callable_params(
      Function& fn,
      FunctionCtx& ctx,
      const Node* callable_node,
      const TypeBindings* tpl_bindings,
      const NttpBindings* nttp_bindings,
      const std::string& context_prefix,
      bool resolve_typedef_struct,
      bool expand_parameter_packs);

  void register_bodyless_callable(Function&& fn);

  bool maybe_register_bodyless_callable(Function* fn, bool has_lowerable_body);

  BlockId begin_callable_body_lowering(Function& fn, FunctionCtx& ctx);

  void finish_lowered_callable(Function* fn, BlockId entry);

  void lower_function(const Node* fn_node,
                      const std::string* name_override = nullptr,
                      const TypeBindings* tpl_override = nullptr,
                      const NttpBindings* nttp_override = nullptr);

  // Lower a struct method as a standalone function with an implicit `this` pointer.
  void lower_struct_method(const std::string& mangled_name,
                           const std::string& struct_tag,
                           const Node* method_node,
                           const TypeBindings* tpl_bindings = nullptr,
                           const NttpBindings* nttp_bindings = nullptr);

  // Hoist a compound literal to an anonymous global variable.
  // Returns the ExprId of an AddrOf(DeclRef{clit_name}) expression.
  ExprId hoist_compound_literal_to_global(const Node* addr_node, const Node* clit);

  void lower_global(const Node* gv);

  void lower_local_decl_stmt(FunctionCtx& ctx, const Node* n);

  std::optional<ExprId> try_lower_consteval_call_expr(FunctionCtx* ctx,
                                                      const Node* n);

  // Check if an AST expression is an lvalue (variable, subscript, deref, member).
  static bool is_ast_lvalue(const Node* n);

  // Resolve a ref-overloaded function call: pick the best overload based on
  // argument value categories. Returns the mangled name of the best match,
  // or empty string if no overload set exists.
  std::string resolve_ref_overload(const std::string& base_name,
                                   const Node* call_node);

  const Node* find_pending_method_by_mangled(
      const std::string& mangled_name) const;

  bool describe_initializer_list_struct(const TypeSpec& ts,
                                        TypeSpec* elem_ts,
                                        TypeSpec* data_ptr_ts,
                                        TypeSpec* len_ts) const;

  ExprId materialize_initializer_list_arg(FunctionCtx* ctx,
                                          const Node* list_node,
                                          const TypeSpec& param_ts);

  ExprId lower_call_expr(FunctionCtx* ctx, const Node* n);

  // Reconstruct the full TypeSpec for a struct field from its HirStructField.
  static TypeSpec field_type_of(const HirStructField& f);

  static TypeSpec field_init_type_of(const HirStructField& f);

  static bool is_char_like(TypeBase base);

  static bool is_scalar_init_type(const TypeSpec& ts);

  static GlobalInit child_init_of(const InitListItem& item);

  static std::optional<InitListItem> make_init_item(const GlobalInit& init);

  bool is_string_scalar(const GlobalInit& init) const;

  long long flat_scalar_count(const TypeSpec& ts) const;

  long long deduce_array_size_from_init(const GlobalInit& init) const;

  TypeSpec resolve_array_ts(const TypeSpec& ts, const GlobalInit& init) const;

  bool is_direct_char_array_init(const TypeSpec& ts, const GlobalInit& init) const;

  bool union_allows_init_normalization(const TypeSpec& ts) const;

  bool struct_allows_init_normalization(const TypeSpec& ts) const;

  GlobalInit normalize_global_init(const TypeSpec& ts, const GlobalInit& init);

  GlobalId lower_static_local_global(FunctionCtx& ctx, const Node* n);

  GlobalInit lower_global_init(const Node* n,
                               FunctionCtx* ctx = nullptr,
                               bool allow_named_const_fold = false);

  InitList lower_init_list(const Node* n, FunctionCtx* ctx = nullptr);

  const Node* init_item_value_node(const Node* item) const;

  const Node* unwrap_init_scalar_value(const Node* node) const;

  bool has_side_effect_expr(const Node* n) const;

  bool is_simple_constant_expr(const Node* n) const;

  bool can_fast_path_scalar_array_init(const Node* init_list) const;

  bool struct_has_member_dtors(const std::string& tag);

  void emit_defaulted_method_body(FunctionCtx& ctx, Function& fn,
                                  const std::string& struct_tag,
                                  const Node* method_node);

  void emit_member_dtor_calls(FunctionCtx& ctx,
                              const std::string& struct_tag,
                              ExprId this_ptr_id,
                              const Node* span_node);

  void emit_dtor_calls(FunctionCtx& ctx, size_t since, const Node* span_node);

  void lower_stmt_node(FunctionCtx& ctx, const Node* n);

  ExprId lower_stmt_expr_block(FunctionCtx& ctx, const Node* block, const TypeSpec& result_ts);

  // Try to resolve an operator expression on a struct type to a member operator
  // method call.  Returns a valid ExprId if the struct has the corresponding
  // operator method, or an invalid ExprId otherwise.
  //
  // `obj_node`      – AST node for the object (LHS for binary/index, operand
  //                   for unary)
  // `op_method_name` – canonical mangled name, e.g. "operator_subscript"
  // `arg_nodes`     – additional argument AST nodes (index for [], RHS for ==,
  //                   etc.)
  // `result_node`   – the top-level expression AST node (for source location)
  ExprId try_lower_operator_call(FunctionCtx* ctx,
                                 const Node* result_node,
                                 const Node* obj_node,
                                 const char* op_method_name,
                                 const std::vector<const Node*>& arg_nodes,
                                 const std::vector<ExprId>& extra_args = {});

  // If the expression resolves to a struct type that has operator_bool,
  // insert an implicit call to operator_bool(). Otherwise return as-is.
  ExprId maybe_bool_convert(FunctionCtx* ctx, ExprId expr, const Node* n);

  TypeSpec builtin_query_result_type() const;

  TypeSpec resolve_builtin_query_type(FunctionCtx* ctx, TypeSpec target) const;

  ExprId lower_builtin_sizeof_type(FunctionCtx* ctx, const Node* n);

  ExprId lower_builtin_alignof_type(FunctionCtx* ctx, const Node* n);

  int builtin_alignof_expr_bytes(FunctionCtx* ctx, const Node* expr);

  ExprId lower_builtin_alignof_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_expr(FunctionCtx* ctx, const Node* n);

  // ── Template multi-instantiation support ──────────────────────────────────

  // AST-side discovered template work item.
  //
  // This is the "todo list" shape we eventually want AST preprocessing to own:
  // a seed describing a discovered template application, not proof that the
  // specialization has been fully realized/lowered yet.
  //
  // TemplateSeedOrigin, TemplateSeedWorkItem, TemplateInstance, and
  // InstantiationRegistry are defined in compile_time_engine.hpp.

  // Build TypeBindings from a call-site template args and a function definition.
  // Resolves typedef args through `enclosing_bindings` if provided.
  // Fills missing args from fn_def's default template parameters.
  TypeBindings build_call_bindings(const Node* call_var, const Node* fn_def,
                                   const TypeBindings* enclosing_bindings);

  NttpBindings build_call_nttp_bindings(const Node* call_var, const Node* fn_def,
                                        const NttpBindings* enclosing_nttp = nullptr);

  // Check if a call node has any forwarded NTTP names (not yet resolved to values).
  static bool has_forwarded_nttp(const Node* call_var);

  // ── Template argument deduction ──────────────────────────────────────────

  // Try to infer the type of an AST expression node for template argument
  // deduction.  Only handles cases where the type is statically obvious
  // from the AST (literals, typed variables, address-of, casts).
  // Returns nullopt when the type cannot be determined.
  static std::optional<TypeSpec> try_infer_arg_type_for_deduction(
      const Node* expr, const Node* enclosing_fn);

  // Try to deduce template type arguments from call arguments.
  // Matches each function parameter whose type is a template type parameter
  // (TB_TYPEDEF with tag matching a template param name) against the
  // corresponding call argument's inferred type.
  // Returns the deduced bindings.  May be incomplete if some params cannot
  // be deduced (caller should fill from defaults or reject).
  static TypeBindings try_deduce_template_type_args(
      const Node* call_node, const Node* fn_def, const Node* enclosing_fn);

  // Check if deduced bindings cover all required type parameters (those
  // without defaults).
  static bool deduction_covers_all_type_params(const TypeBindings& deduced,
                                               const Node* fn_def);

  // Fill missing deduced bindings from defaults.
  static void fill_deduced_defaults(TypeBindings& deduced, const Node* fn_def);

  TypeBindings merge_explicit_and_deduced_type_bindings(
      const Node* call_node, const Node* call_var, const Node* fn_def,
      const TypeBindings* enclosing_bindings = nullptr,
      const Node* enclosing_fn = nullptr);

  // ── End template argument deduction ────────────────────────────────────

  // Check if all bindings are concrete (no unresolved TB_TYPEDEF).

  // Get template parameter names in declaration order.
  static std::vector<std::string> get_template_param_order(
      const Node* fn_def,
      const TypeBindings* bindings = nullptr,
      const NttpBindings* nttp_bindings = nullptr) {
    std::vector<std::string> params;
    if (!fn_def) return params;
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      const std::string pname = fn_def->template_param_names[i];
      const bool is_pack =
          fn_def->template_param_is_pack && fn_def->template_param_is_pack[i];
      if (!is_pack) {
        params.push_back(pname);
        continue;
      }
      std::vector<std::pair<int, std::string>> bound_pack_names;
      if (bindings) {
        for (const auto& [key, _] : *bindings) {
          int pack_index = 0;
          if (parse_pack_binding_name(key, pname, &pack_index))
            bound_pack_names.push_back({pack_index, key});
        }
      }
      if (nttp_bindings) {
        for (const auto& [key, _] : *nttp_bindings) {
          int pack_index = 0;
          if (parse_pack_binding_name(key, pname, &pack_index))
            bound_pack_names.push_back({pack_index, key});
        }
      }
      std::sort(bound_pack_names.begin(), bound_pack_names.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
      for (const auto& [_, key] : bound_pack_names) params.push_back(key);
    }
    return params;
  }

  // Get template parameter order from the definition (for specialization key).
  std::vector<std::string> get_template_param_order_from_instances(
      const std::string& fn_name);

  // Record a template seed via the centralized registry.
  // Returns the mangled name, or "" if bindings are not concrete.
  std::string record_seed(
      const std::string& fn_name,
      TypeBindings bindings,
      NttpBindings nttp_bindings = {},
      TemplateSeedOrigin origin = TemplateSeedOrigin::DirectCall);

  // Resolve the mangled name for a call to a template function.
  std::string resolve_template_call_name(
      const Node* call_var,
      const TypeBindings* enclosing_bindings,
      const NttpBindings* enclosing_nttp = nullptr);

  // Recursively collect template instantiations from call sites in AST.
  void collect_template_instantiations(const Node* n, const Node* enclosing_fn);

  // Like collect_template_instantiations, but only records instances for
  // consteval template functions.  Non-consteval template calls inside template
  // bodies are left for the compile-time reduction pass to discover and lower.
  void collect_consteval_template_instantiations(
      const Node* n, const Node* enclosing_fn);

  // Check if a template function is called from any non-template function
  // without explicit template args (implicit deduction / plain call).
  static bool is_referenced_without_template_args(
      const char* fn_name, const std::vector<const Node*>& items);

  static bool has_plain_call(const Node* n, const char* fn_name);

  const Node* find_template_struct_primary(const std::string& name) const;
  const std::vector<const Node*>* find_template_struct_specializations(
      const Node* primary_tpl) const;
  TemplateStructEnv build_template_struct_env(const Node* primary_tpl) const;
  void register_template_struct_primary(const std::string& name, const Node* node);
  void register_template_struct_specialization(const Node* primary_tpl,
                                               const Node* node);

  Module* module_ = nullptr;
  std::unordered_map<std::string, long long> enum_consts_;
  std::unordered_map<std::string, long long> const_int_bindings_;
  std::unordered_set<std::string> weak_symbols_;
  // Engine-owned compile-time state.  Shared with the pipeline so
  // the compile-time engine can access the registry directly.
  std::shared_ptr<CompileTimeState> ct_state_ = std::make_shared<CompileTimeState>();
  // Convenience alias — shorthand for ct_state_->registry.
  InstantiationRegistry& registry_ = ct_state_->registry;
  // Template struct definitions indexed by struct tag name.
  std::unordered_map<std::string, const Node*> struct_def_nodes_;
  std::unordered_map<std::string, const Node*> template_struct_defs_;
  // Template struct specializations indexed by primary template name.
  std::unordered_map<std::string, std::vector<const Node*>> template_struct_specializations_;
  // Already-instantiated template structs, keyed by semantic identity
  // (primary template + concrete bindings), not by printed/mangled name.
  std::unordered_set<TemplateStructInstanceKey, TemplateStructInstanceKeyHash>
      instantiated_tpl_struct_keys_;
  // Static data members indexed by owning struct tag and member name.
  std::unordered_map<std::string, std::unordered_map<std::string, const Node*>> struct_static_member_decls_;
  std::unordered_map<std::string, std::unordered_map<std::string, long long>>
      struct_static_member_const_values_;
  // Struct method map: "struct_tag::method_name" → mangled function name.
  std::unordered_map<std::string, std::string> struct_methods_;
  // Struct method return types: "struct_tag::method_name" → return TypeSpec.
  std::unordered_map<std::string, TypeSpec> struct_method_ret_types_;
  // Pending struct methods to lower.
  struct PendingMethod {
    std::string mangled;
    std::string struct_tag;
    const Node* method_node;
    TypeBindings tpl_bindings;
    NttpBindings nttp_bindings;
  };
  std::vector<PendingMethod> pending_methods_;
  // Deduced template call info: call_node → mangled name + bindings.
  struct DeducedTemplateCall {
    std::string mangled_name;
    TypeBindings bindings;
    NttpBindings nttp_bindings;
  };
  std::unordered_map<const Node*, DeducedTemplateCall> deduced_template_calls_;
  // Constructor overloads per struct tag: tag → list of {mangled, method_node}.
  struct CtorOverload {
    std::string mangled_name;
    const Node* method_node;  // for parameter type matching
  };
  std::unordered_map<std::string, std::vector<CtorOverload>> struct_constructors_;
  // Destructor per struct tag: tag → {mangled, method_node}.
  struct DtorInfo {
    std::string mangled_name;
    const Node* method_node;
  };
  std::unordered_map<std::string, DtorInfo> struct_destructors_;
  // Ref-overload resolution: base function name → list of overload entries.
  struct RefOverloadEntry {
    std::string mangled_name;
    std::vector<bool> param_is_rvalue_ref;
    std::vector<bool> param_is_lvalue_ref;
  };
  std::unordered_map<std::string, std::vector<RefOverloadEntry>> ref_overload_set_;
  // Reverse mapping: AST Node* of overloaded function → mangled name.
  std::unordered_map<const Node*, std::string> ref_overload_mangled_;
  // Marks lowering performed on behalf of the compile-time engine so pending
  // consteval nodes can preserve deferred-instantiation provenance.
  bool lowering_deferred_instantiation_ = false;

};

}  // namespace c4c::hir
