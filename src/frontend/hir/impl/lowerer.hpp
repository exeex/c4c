#pragma once

// Private HIR Lowerer engine index.
//
// Role:
// - declares the implementation-only Lowerer engine shared across HIR lowering
//   implementation translation units
// - groups helper families by lowering phase and semantic responsibility
// - provides the internal navigation index for callable, stmt, expr, template,
//   and initializer lowering code
//
// Implementation map:
// - root implementation files: pipeline orchestration, callable/global
//   lowering, type/layout/init normalization, and shared source helpers
// - impl/templates/templates.hpp: template/dependent lowering implementation index
// - impl/compile_time/compile_time.hpp: compile-time/materialization implementation index
// - impl/stmt/stmt.hpp: statement lowering implementation index
// - impl/expr/expr.hpp: expression lowering implementation index

#include "hir_impl.hpp"
#include "../../sema/consteval.hpp"
#include "../../sema/type_utils.hpp"
#include "../../parser/parser_support.hpp"

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
std::string encode_template_type_arg_ref_hir(const TypeSpec& ts);
bool eval_struct_static_member_value_hir(
    const Node* sdef,
    const std::unordered_map<std::string, const Node*>& struct_defs,
    const std::string& member_name,
    const NttpBindings* nttp_bindings,
    long long* out);

class Lowerer {
 public:
  // ── pipeline entry state ──────────────────────────────────────────────────
  const sema::ResolvedTypeTable* resolved_types_ = nullptr;

  /// Engine-owned compile-time state, shared with the pipeline.
  std::shared_ptr<CompileTimeState> ct_state() const;

  // ── deferred template-type seeding and resolution ────────────────────────
  void seed_pending_template_type(const TypeSpec& ts,
                                  const TypeBindings& tpl_bindings,
                                  const NttpBindings& nttp_bindings,
                                  const Node* span_node,
                                  PendingTemplateTypeKind kind,
                                  const std::string& context_name = {});

  const Node* canonical_template_struct_primary(
      const TypeSpec& ts,
      const Node* primary_tpl = nullptr) const;

  void realize_template_struct_if_needed(
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

  // ── initial build orchestration and collection passes ────────────────────
  std::vector<const Node*> flatten_program_items(const Node* root) const;

  void collect_weak_symbol_names(const std::vector<const Node*>& items);

  void collect_enum_def(const Node* ed, bool register_structured_globals = false);

  void collect_initial_type_definitions(const std::vector<const Node*>& items);

  void collect_consteval_function_definitions(const std::vector<const Node*>& items);

  void collect_template_function_definitions(const std::vector<const Node*>& items);

  void collect_function_template_specializations(const std::vector<const Node*>& items);

  void collect_template_global_definitions(const std::vector<const Node*>& items);

  void collect_depth0_template_instantiations(const std::vector<const Node*>& items);

  void realize_consteval_template_seeds_fixpoint(
      const std::vector<const Node*>& items);

  void finalize_template_seed_realization();

  void materialize_hir_template_defs(Module& m);

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
                                     const NttpTextBindings& nttp_bindings_by_text,
                                     const std::string& mangled);

  DeferredTemplateTypeResult instantiate_deferred_template_type(
      const PendingTemplateTypeWorkItem& work_item);

 private:
  // ── per-function lowering state ──────────────────────────────────────────
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
    // Step 5 classification: function-scope parser spelling maps. These are
    // local-scope lowering state, not module/global semantic lookup authority.
    std::unordered_map<std::string, LocalId> locals;
    DenseIdMap<LocalId, TypeSpec> local_types;
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
    std::unordered_map<TextId, TypeSpec> tpl_bindings_by_text;
    NttpBindings nttp_bindings; // non-type template param → constant value
    NttpTextBindings nttp_bindings_by_text; // non-type template param TextId → value
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

  std::optional<long long> lookup_nttp_binding(const FunctionCtx* ctx,
                                               const Node* name_node,
                                               const char* rendered_name) const;

  static TypeSpec reference_storage_ts(TypeSpec ts);

  static TypeSpec reference_value_ts(TypeSpec ts);

  // ── low-level type and id utilities ──────────────────────────────────────
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
      const FunctionCtx* ctx, const Node* callee);

  std::optional<TypeSpec> infer_call_result_type(
      const FunctionCtx* ctx, const Node* call);

  std::optional<TypeSpec> storage_type_for_declref(FunctionCtx* ctx,
                                                   const DeclRef& r);

  void attach_decl_ref_link_name_id(DeclRef& ref) const;

  std::optional<ExprId> try_lower_rvalue_ref_storage_addr(
      FunctionCtx* ctx,
      const Node* n,
      const TypeSpec& storage_ts);

  // ── block / expression construction helpers ──────────────────────────────
  TypeSpec infer_generic_ctrl_type(FunctionCtx* ctx, const Node* n);

  Block& ensure_block(FunctionCtx& ctx, BlockId id);

  BlockId create_block(FunctionCtx& ctx);

  void append_stmt(FunctionCtx& ctx, Stmt stmt);

  const Node* find_struct_static_member_decl(const std::string& tag,
                                             const std::string& member) const;
  const Node* find_struct_static_member_decl(
      const HirStructMemberLookupKey& key,
      const std::string* rendered_tag = nullptr,
      const std::string* rendered_member = nullptr) const;

  std::optional<long long> find_struct_static_member_const_value(
      const std::string& tag, const std::string& member) const;
  std::optional<long long> find_struct_static_member_const_value(
      const HirStructMemberLookupKey& key,
      const std::string* rendered_tag = nullptr,
      const std::string* rendered_member = nullptr) const;
  MemberSymbolId find_struct_member_symbol_id(
      const std::string& tag, const std::string& member) const;
  MemberSymbolId find_struct_member_symbol_id(
      const HirStructMemberLookupKey& key,
      const std::string* rendered_tag = nullptr,
      const std::string* rendered_member = nullptr) const;
  MemberSymbolId find_struct_member_symbol_id(
      const TypeSpec& owner_ts,
      const std::string& rendered_tag,
      const std::string& member,
      TextId member_text_id) const;
  std::optional<long long> try_eval_instantiated_struct_static_member_const(
      const std::string& tag, const std::string& member) const;

  long long eval_const_int_with_nttp_bindings(
      const Node* n,
      const NttpBindings& nttp_bindings,
      const NttpTextBindings* nttp_bindings_by_text = nullptr) const;

  std::optional<std::string> find_struct_method_mangled(
      const std::string& tag,
      const std::string& method,
      bool is_const_obj) const;
  std::optional<LinkNameId> find_struct_method_link_name_id(
      const std::string& tag,
      const std::string& method,
      bool is_const_obj) const;

  std::optional<TypeSpec> find_struct_method_return_type(
      const std::string& tag,
      const std::string& method,
      bool is_const_obj) const;
  std::string resolve_struct_method_lookup_owner_tag(
      const TypeSpec& owner_ts,
      bool is_arrow,
      const TypeBindings* tpl_bindings,
      const NttpBindings* nttp_bindings,
      const std::string* current_struct_tag,
      const Node* span_node,
      const std::string& context_name);
  std::optional<HirStructMethodLookupKey> make_struct_method_lookup_key(
      const std::string& tag,
      const std::string& method,
      bool is_const_method) const;
  std::optional<HirStructMemberLookupKey> make_struct_member_lookup_key(
      const std::string& tag,
      const std::string& member) const;
  std::optional<HirStructMemberLookupKey> make_struct_member_lookup_key(
      const HirRecordOwnerKey& owner_key,
      TextId member_text_id) const;
  std::optional<HirStructMemberLookupKey> make_struct_member_lookup_key(
      const TypeSpec& owner_ts,
      TextId member_text_id) const;
  void record_struct_method_mangled_lookup_parity(
      const std::string& tag,
      const std::string& method,
      bool is_const_method,
      const std::string& rendered_mangled) const;
  void record_struct_method_link_name_lookup_parity(
      const std::string& tag,
      const std::string& method,
      bool is_const_method,
      LinkNameId rendered_link_name_id) const;
  void record_struct_method_return_type_lookup_parity(
      const std::string& tag,
      const std::string& method,
      bool is_const_method,
      const TypeSpec& rendered_return_type) const;
  void record_struct_static_member_decl_lookup_parity(
      const std::string& tag,
      const std::string& member,
      const Node* rendered_decl) const;
  void record_struct_static_member_const_value_lookup_parity(
      const std::string& tag,
      const std::string& member,
      long long rendered_value) const;
  void record_struct_member_symbol_id_lookup_parity(
      const std::string& tag,
      const std::string& member,
      MemberSymbolId rendered_member_symbol_id) const;

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

  std::optional<HirRecordOwnerKey> make_struct_def_node_owner_key(
      const Node* sd) const;
  void register_struct_def_node_owner(const Node* sd);
  void register_struct_method_owner_lookup(
      const HirRecordOwnerKey& owner_key,
      const Node* method,
      bool is_const_method,
      const std::string& rendered_key,
      const std::string& mangled,
      const TypeSpec& return_type);
  void register_struct_static_member_owner_lookup(
      const HirRecordOwnerKey& owner_key,
      const Node* member,
      const std::optional<long long>& const_value = std::nullopt);
  void register_struct_member_symbol_owner_lookup(
      const HirRecordOwnerKey& owner_key,
      TextId member_text_id,
      MemberSymbolId member_symbol_id);
  std::optional<HirRecordOwnerKey> make_template_struct_instance_owner_key(
      const HirStructDef& def,
      const Node* primary_tpl,
      const TemplateStructInstanceKey& instance_key) const;
  std::optional<HirRecordOwnerKey> register_template_struct_instance_owner(
      const HirStructDef& def,
      const Node* primary_tpl,
      const Node* struct_node,
      const TemplateStructInstanceKey& instance_key,
      bool append_order);

  ResolvedTemplateArgs materialize_template_args(
      const Node* primary_tpl,
      const TypeSpec& owner_ts,
      const TypeBindings& tpl_bindings,
      const NttpBindings& nttp_bindings);

  // ── template-struct realization helpers ──────────────────────────────────
  void lower_struct_def(const Node* sd);

  bool resolve_struct_member_typedef_type(const std::string& tag,
                                          const std::string& member,
                                          TypeSpec* out);

  PreparedTemplateStructInstance prepare_template_struct_instance(
      const Node* primary_tpl,
      const char* origin,
      const ResolvedTemplateArgs& resolved);

  static std::string build_template_mangled_name(
      const Node* primary_tpl,
      const Node* tpl_def,
      const char* origin,
      const ResolvedTemplateArgs& resolved);

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
      const std::optional<HirRecordOwnerKey>& owner_key,
      const Node* tpl_def,
      const TypeBindings& method_tpl_bindings,
      const NttpBindings& method_nttp_bindings);

  void record_instantiated_template_struct_field_metadata(
      const std::string& mangled,
      const std::optional<HirRecordOwnerKey>& owner_key,
      const Node* orig_f,
      const NttpBindings& selected_nttp_bindings_map,
      const NttpTextBindings* selected_nttp_bindings_by_text = nullptr);

  std::optional<HirStructField> instantiate_template_struct_field(
      const Node* orig_f,
      const std::string& owner_tag,
      const std::optional<HirRecordOwnerKey>& owner_key,
      const TypeBindings& selected_type_bindings,
      const NttpBindings& selected_nttp_bindings_map,
      const Node* tpl_def,
      int llvm_idx);

  void append_instantiated_template_struct_fields(
      HirStructDef& def,
      const std::string& mangled,
      const std::optional<HirRecordOwnerKey>& owner_key,
      const Node* tpl_def,
      const TypeBindings& selected_type_bindings,
      const NttpBindings& selected_nttp_bindings_map,
      const NttpTextBindings* selected_nttp_bindings_by_text = nullptr);

  void instantiate_template_struct_body(
      const std::string& mangled,
      const Node* primary_tpl,
      const Node* tpl_def,
      const SelectedTemplateStructPattern& selected_pattern,
      const std::vector<HirTemplateArg>& concrete_args,
      const TemplateStructInstanceKey& instance_key);

  void assign_template_arg_refs_from_ast_args(
      TypeSpec* ts,
      const Node* ref,
      const Node* owner_tpl,
      const FunctionCtx* ctx,
      const Node* span_node,
      PendingTemplateTypeKind kind,
      const std::string& context_name);

  bool recover_template_struct_identity_from_tag(
      TypeSpec* ts,
      const std::string* current_struct_tag = nullptr) const;

  std::optional<std::string> resolve_member_lookup_owner_tag(
      TypeSpec base_ts,
      bool is_arrow,
      const TypeBindings* tpl_bindings,
      const NttpBindings* nttp_bindings,
      const std::string* current_struct_tag,
      const Node* span_node,
      const std::string& context_name);

  bool resolve_ast_template_value_arg(
      const Node* owner_tpl,
      const Node* ref,
      int index,
      const FunctionCtx* ctx,
      long long* out_value,
      const char** out_debug_ref = nullptr);
  bool try_eval_template_value_arg_expr(
      const Node* expr,
      const FunctionCtx* ctx,
      long long* out_value);

  void realize_template_struct(TypeSpec& ts,
                               const Node* primary_tpl,
                               const TypeBindings& tpl_bindings,
                               const NttpBindings& nttp_bindings);

  // ── callable and global lowering helpers ─────────────────────────────────
  TypeSpec substitute_signature_template_type(
      TypeSpec ts, const TypeBindings* tpl_bindings);

  void resolve_signature_template_type_if_needed(
      TypeSpec& ts,
      const TypeBindings* tpl_bindings,
      const NttpBindings* nttp_bindings,
      const std::string* current_struct_tag,
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
                      const NttpBindings* nttp_override = nullptr,
                      const NttpTextBindings* nttp_text_override = nullptr);

  // Lower a struct method as a standalone function with an implicit `this` pointer.
  void lower_struct_method(const std::string& mangled_name,
                           const std::string& struct_tag,
                           const Node* method_node,
                           const TypeBindings* tpl_bindings = nullptr,
                           const NttpBindings* nttp_bindings = nullptr,
                           const NttpTextBindings* nttp_text_bindings = nullptr);

  // Hoist a compound literal to an anonymous global variable.
  // Returns the ExprId of an AddrOf(DeclRef{clit_name}) expression.
  ExprId hoist_compound_literal_to_global(const Node* addr_node, const Node* clit);

  void lower_global(const Node* gv,
                    const std::string* name_override = nullptr,
                    const TypeBindings* tpl_override = nullptr,
                    const NttpBindings* nttp_override = nullptr,
                    const NttpTextBindings* nttp_text_override = nullptr);

  void lower_local_decl_stmt(FunctionCtx& ctx, const Node* n);

  std::optional<ExprId> try_lower_consteval_call_expr(FunctionCtx* ctx,
                                                      const Node* n);

  // Check if an AST expression is an lvalue (variable, subscript, deref, member).
  static bool is_ast_lvalue(const Node* n, const FunctionCtx* ctx = nullptr);

  // Resolve a ref-overloaded function call: pick the best overload based on
  // argument value categories. Returns the mangled name of the best match,
  // or empty string if no overload set exists.
  std::string resolve_ref_overload(const std::string& base_name,
                                   const Node* call_node,
                                   const FunctionCtx* ctx = nullptr);

  const Node* find_pending_method_by_mangled(
      const std::string& mangled_name) const;

  bool describe_initializer_list_struct(const TypeSpec& ts,
                                        TypeSpec* elem_ts,
                                        TypeSpec* data_ptr_ts,
                                        TypeSpec* len_ts) const;

  ExprId materialize_initializer_list_arg(FunctionCtx* ctx,
                                          const Node* list_node,
                                          const TypeSpec& param_ts);

  std::optional<ExprId> try_lower_direct_struct_constructor_call(FunctionCtx* ctx,
                                                                 const Node* n);

  std::optional<ExprId> try_lower_template_struct_call(FunctionCtx* ctx,
                                                       const Node* n);

  ExprId lower_call_arg(FunctionCtx* ctx,
                        const Node* arg_node,
                        const TypeSpec* param_ts);

  bool try_expand_pack_call_arg(FunctionCtx* ctx,
                                CallExpr& call,
                                const Node* arg_node,
                                const TypeSpec* construct_param_ts);

  std::optional<ExprId> try_lower_member_call_expr(FunctionCtx* ctx,
                                                   const Node* n);

  ExprId lower_call_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_member_expr(FunctionCtx* ctx, const Node* n);

  // ── initializer and aggregate lowering helpers ───────────────────────────
  // Reconstruct the full TypeSpec for a struct field from its HirStructField.
  static TypeSpec field_type_of(const HirStructField& f);

  static TypeSpec field_init_type_of(const HirStructField& f);

  static bool is_char_like(TypeBase base);

  static bool is_scalar_init_type(const TypeSpec& ts);

  static GlobalInit child_init_of(const InitListItem& item);

  static std::optional<InitListItem> make_init_item(const GlobalInit& init);

  bool is_string_scalar(const GlobalInit& init) const;

  const HirStructDef* find_struct_def_for_layout_type(const TypeSpec& ts) const;

  const HirStructField* find_struct_instance_field_including_bases(
      const TypeSpec& owner_ts,
      const std::string& field) const;

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

  void lower_range_for_stmt(FunctionCtx& ctx, const Node* n);

  void lower_if_stmt(FunctionCtx& ctx, const Node* n);

  void lower_while_stmt(FunctionCtx& ctx, const Node* n);

  void lower_for_stmt(FunctionCtx& ctx, const Node* n);

  void lower_do_while_stmt(FunctionCtx& ctx, const Node* n);

  void lower_switch_stmt(FunctionCtx& ctx, const Node* n);

  void lower_case_stmt(FunctionCtx& ctx, const Node* n);

  void lower_case_range_stmt(FunctionCtx& ctx, const Node* n);

  void lower_default_stmt(FunctionCtx& ctx, const Node* n);

  void attach_switch_cases(FunctionCtx& ctx, BlockId parent_b);

  void branch_to_block_if_needed(FunctionCtx& ctx, BlockId target, const Node* n);

  void lower_stmt_node(FunctionCtx& ctx, const Node* n);

  ExprId lower_stmt_expr_block(FunctionCtx& ctx, const Node* block, const TypeSpec& result_ts);

  // ── expression lowering helpers ──────────────────────────────────────────
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
  void populate_template_type_text_bindings(FunctionCtx& ctx,
                                            const Node* template_owner,
                                            const TypeBindings* bindings) const;

  ExprId lower_builtin_sizeof_type(FunctionCtx* ctx, const Node* n);

  ExprId lower_builtin_alignof_type(FunctionCtx* ctx, const Node* n);

  int builtin_alignof_expr_bytes(FunctionCtx* ctx, const Node* expr);

  ExprId lower_builtin_alignof_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_var_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_unary_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_postfix_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_addr_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_deref_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_comma_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_binary_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_assign_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_compound_assign_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_cast_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_va_arg_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_index_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_ternary_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_generic_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_stmt_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_complex_part_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_sizeof_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_sizeof_pack_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_compound_literal_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_new_expr(FunctionCtx* ctx, const Node* n);

  ExprId lower_delete_expr(FunctionCtx* ctx, const Node* n);

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
                                        const NttpBindings* enclosing_nttp = nullptr,
                                        const NttpTextBindings* enclosing_nttp_by_text = nullptr);

  NttpTextBindings build_call_nttp_text_bindings(const Node* call_var,
                                                 const Node* fn_def,
                                                 const NttpBindings& nttp_bindings);

  // Check if a call node has any forwarded NTTP names (not yet resolved to values).
  static bool has_forwarded_nttp(const Node* call_var);

  // ── Template argument deduction ──────────────────────────────────────────

  // Try to infer the type of an AST expression node for template argument
  // deduction.  Only handles cases where the type is statically obvious
  // from the AST (literals, typed variables, address-of, casts, and calls with
  // inferable result types).
  // Returns nullopt when the type cannot be determined.
  std::optional<TypeSpec> try_infer_arg_type_for_deduction(
      const Node* expr, const Node* enclosing_fn);

  // Try to deduce template type arguments from call arguments.
  // Matches each function parameter whose type is a template type parameter
  // (TB_TYPEDEF with tag matching a template param name) against the
  // corresponding call argument's inferred type.
  // Returns the deduced bindings.  May be incomplete if some params cannot
  // be deduced (caller should fill from defaults or reject).
  TypeBindings try_deduce_template_type_args(
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

  // Record a template-instantiation seed via the centralized registry.
  // Returns the mangled name, or "" if bindings are not concrete.
  std::string record_template_seed(
      const std::string& fn_name,
      TypeBindings bindings,
      NttpBindings nttp_bindings = {},
      NttpTextBindings nttp_bindings_by_text = {},
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

  // ── template registry and lowering-owned caches ──────────────────────────
  const Node* find_template_struct_primary(const std::string& name) const;
  const Node* find_template_struct_primary(
      const Node* declaration,
      const std::string& rendered_name = {}) const;
  const std::vector<const Node*>* find_template_struct_specializations(
      const Node* primary_tpl) const;
  void record_template_struct_primary_lookup_parity(
      const Node* rendered_primary) const;
  void record_template_struct_specialization_lookup_parity(
      const Node* primary_tpl,
      const std::vector<const Node*>* rendered_specializations) const;
  const Node* find_template_global_primary(const std::string& name) const;
  const std::vector<const Node*>* find_template_global_specializations(
      const Node* primary_tpl) const;
  std::optional<long long> try_eval_template_static_member_const(
      FunctionCtx* ctx,
      const std::string& tpl_name,
      const Node* ref,
      const std::string& member);
  std::optional<GlobalId> ensure_template_global_instance(FunctionCtx* ctx,
                                                          const Node* ref);
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
  // Step 5 classification: rendered-name compatibility caches below are kept
  // beside structured mirrors/parity checks until all HIR producers and
  // consumers can pass owner/member identity directly.
  // Template struct definitions indexed by struct tag name.
  std::unordered_map<std::string, const Node*> struct_def_nodes_;
  std::unordered_map<HirRecordOwnerKey, const Node*, HirRecordOwnerKeyHash>
      struct_def_nodes_by_owner_;
  std::unordered_map<const Node*, HirRecordOwnerKey> struct_def_owner_by_node_;
  std::unordered_map<std::string, const Node*> template_struct_defs_;
  std::unordered_map<HirRecordOwnerKey, const Node*, HirRecordOwnerKeyHash>
      template_struct_defs_by_owner_;
  mutable size_t template_struct_primary_lookup_parity_checks_ = 0;
  mutable size_t template_struct_primary_lookup_parity_mismatches_ = 0;
  // Template struct specializations indexed by primary template name.
  std::unordered_map<std::string, std::vector<const Node*>> template_struct_specializations_;
  std::unordered_map<HirRecordOwnerKey, std::vector<const Node*>, HirRecordOwnerKeyHash>
      template_struct_specializations_by_owner_;
  mutable size_t template_struct_specialization_lookup_parity_checks_ = 0;
  mutable size_t template_struct_specialization_lookup_parity_mismatches_ = 0;
  std::unordered_map<std::string, const Node*> template_global_defs_;
  std::unordered_map<std::string, std::vector<const Node*>> template_global_specializations_;
  std::unordered_map<TemplateStructInstanceKey, GlobalId, TemplateStructInstanceKeyHash>
      instantiated_template_globals_;
  // Already-instantiated template structs, keyed by semantic identity
  // (primary template + concrete bindings), not by printed/mangled name.
  std::unordered_set<TemplateStructInstanceKey, TemplateStructInstanceKeyHash>
      instantiated_tpl_struct_keys_;
  // Static data members indexed by owning struct tag and member name.
  std::unordered_map<std::string, std::unordered_map<std::string, const Node*>> struct_static_member_decls_;
  std::unordered_map<std::string, std::unordered_map<std::string, long long>>
      struct_static_member_const_values_;
  std::unordered_map<HirStructMemberLookupKey, const Node*, HirStructMemberLookupKeyHash>
      struct_static_member_decls_by_owner_;
  mutable size_t struct_static_member_decl_lookup_parity_checks_ = 0;
  mutable size_t struct_static_member_decl_lookup_parity_mismatches_ = 0;
  std::unordered_map<HirStructMemberLookupKey, long long, HirStructMemberLookupKeyHash>
      struct_static_member_const_values_by_owner_;
  mutable size_t struct_static_member_const_value_lookup_parity_checks_ = 0;
  mutable size_t struct_static_member_const_value_lookup_parity_mismatches_ = 0;
  std::unordered_map<HirStructMemberLookupKey, MemberSymbolId, HirStructMemberLookupKeyHash>
      struct_member_symbol_ids_by_owner_;
  mutable size_t struct_member_symbol_id_lookup_parity_checks_ = 0;
  mutable size_t struct_member_symbol_id_lookup_parity_mismatches_ = 0;
  // Struct method map: "struct_tag::method_name" → mangled function name.
  std::unordered_map<std::string, std::string> struct_methods_;
  std::unordered_map<HirStructMethodLookupKey, std::string, HirStructMethodLookupKeyHash>
      struct_methods_by_owner_;
  mutable size_t struct_method_mangled_lookup_parity_checks_ = 0;
  mutable size_t struct_method_mangled_lookup_parity_mismatches_ = 0;
  // Struct method map: "struct_tag::method_name" → link-visible symbol id.
  std::unordered_map<std::string, LinkNameId> struct_method_link_name_ids_;
  std::unordered_map<HirStructMethodLookupKey, LinkNameId, HirStructMethodLookupKeyHash>
      struct_method_link_name_ids_by_owner_;
  mutable size_t struct_method_link_name_lookup_parity_checks_ = 0;
  mutable size_t struct_method_link_name_lookup_parity_mismatches_ = 0;
  // Struct method return types: "struct_tag::method_name" → return TypeSpec.
  std::unordered_map<std::string, TypeSpec> struct_method_ret_types_;
  std::unordered_map<HirStructMethodLookupKey, TypeSpec, HirStructMethodLookupKeyHash>
      struct_method_ret_types_by_owner_;
  mutable size_t struct_method_return_type_lookup_parity_checks_ = 0;
  mutable size_t struct_method_return_type_lookup_parity_mismatches_ = 0;
  // Top-level function definitions keyed by unqualified name for early
  // template-deduction return-type probes before ordinary functions are
  // lowered into the module; this is a lowering-phase unresolved boundary.
  std::unordered_map<std::string, const Node*> function_decl_nodes_;
  // Pending struct methods to lower.
  struct PendingMethod {
    std::string mangled;
    std::string struct_tag;
    const Node* method_node;
    TypeBindings tpl_bindings;
    NttpBindings nttp_bindings;
    NttpTextBindings nttp_bindings_by_text;
  };

  struct LowererConstEvalStructuredMaps {
    ConstStructuredMap enum_consts_by_key;
    ConstStructuredMap named_consts_by_key;
  };

  void refresh_global_consteval_structured_maps(
      LowererConstEvalStructuredMaps& maps) const;
  ConstEvalEnv make_lowerer_consteval_env(
      LowererConstEvalStructuredMaps& maps,
      const ConstMap* local_consts = nullptr,
      bool include_named_consts = true) const;
  std::vector<PendingMethod> pending_methods_;
  // Deduced template call info: call_node → mangled name + bindings.
  struct DeducedTemplateCall {
    std::string mangled_name;
    TypeBindings bindings;
    NttpBindings nttp_bindings;
    NttpTextBindings nttp_bindings_by_text;
  };
  std::unordered_map<const Node*, DeducedTemplateCall> deduced_template_calls_;
  std::unordered_set<const Node*> rejected_template_calls_;
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
    bool method_is_lvalue_ref = false;
    bool method_is_rvalue_ref = false;
  };
  std::unordered_map<std::string, std::vector<RefOverloadEntry>> ref_overload_set_;
  // Reverse mapping: AST Node* of overloaded function → mangled name.
  std::unordered_map<const Node*, std::string> ref_overload_mangled_;
  // Marks lowering performed on behalf of the compile-time engine so pending
  // consteval nodes can preserve deferred-instantiation provenance.
  bool lowering_deferred_instantiation_ = false;
};

}  // namespace c4c::hir
