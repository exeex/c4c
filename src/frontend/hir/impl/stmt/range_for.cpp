#include "stmt.hpp"

#include <cstring>
#include <optional>
#include <stdexcept>

namespace c4c::hir {

namespace {

bool is_range_for_generated_anonymous_record_tag(const char* name) {
  return name && std::strncmp(name, "_anon_", 6) == 0;
}

NamespaceQualifier range_for_type_ns_qual(const TypeSpec& ts) {
  NamespaceQualifier ns_qual;
  ns_qual.context_id = ts.namespace_context_id;
  ns_qual.is_global_qualified = ts.is_global_qualified;
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(
        ts.qualifier_text_ids,
        ts.qualifier_text_ids + ts.n_qualifier_segments);
  }
  return ns_qual;
}

}  // namespace

void Lowerer::lower_range_for_stmt(FunctionCtx& ctx, const Node* n) {
  const Node* decl_node = n->init;
  const Node* range_node = n->right;

  struct RangeForOwner {
    std::string tag;
    std::optional<HirRecordOwnerKey> owner_key;
    bool has_structured_identity = false;
  };

  auto has_structured_owner_identity = [&](const TypeSpec& owner_ts) {
    const std::optional<HirRecordOwnerKey> record_owner_key =
        (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF)
            ? make_struct_def_node_owner_key(owner_ts.record_def)
            : std::nullopt;
    const bool record_def_has_structured_owner_identity =
        record_owner_key.has_value() &&
        !is_range_for_generated_anonymous_record_tag(owner_ts.record_def->name);
    return record_def_has_structured_owner_identity ||
           (owner_ts.namespace_context_id >= 0 &&
            owner_ts.tag_text_id != kInvalidText) ||
           (owner_ts.tpl_struct_origin && owner_ts.tpl_struct_origin[0]) ||
           (owner_ts.tpl_struct_args.data && owner_ts.tpl_struct_args.size > 0);
  };

  auto diagnostic_owner_name = [&](const TypeSpec& owner_ts,
                                   const RangeForOwner& owner) {
    if (!owner.tag.empty()) return owner.tag;
    if (owner_ts.record_def && owner_ts.record_def->name &&
        owner_ts.record_def->name[0]) {
      return std::string(owner_ts.record_def->name);
    }
    if (module_ && module_->link_name_texts &&
        owner_ts.tag_text_id != kInvalidText) {
      const std::string_view text =
          module_->link_name_texts->lookup(owner_ts.tag_text_id);
      if (!text.empty()) return std::string(text);
    }
    if (owner.owner_key) return owner.owner_key->debug_label();
    return std::string("<unknown>");
  };

  auto resolve_structured_owner =
      [&](TypeSpec owner_ts,
          const std::string& context_name) -> RangeForOwner {
    RangeForOwner owner;
    owner.has_structured_identity = has_structured_owner_identity(owner_ts);
    while (resolve_struct_member_typedef_if_ready(&owner_ts)) {
    }
    resolve_typedef_to_struct(owner_ts);
    if ((owner_ts.base != TB_STRUCT && owner_ts.base != TB_UNION) ||
        owner_ts.ptr_level != 0 || owner_ts.array_rank != 0) {
      return owner;
    }

    auto resolve_from_key = [&](const HirRecordOwnerKey& key) -> bool {
      if (!hir_record_owner_key_has_complete_metadata(key)) return false;
      if (const SymbolName* tag = module_->find_struct_def_tag_by_owner(key)) {
        if (module_->struct_defs.count(*tag)) {
          owner.owner_key = key;
          owner.tag = *tag;
          return true;
        }
      }
      return false;
    };

    if (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF) {
      if (const std::optional<HirRecordOwnerKey> key =
              make_struct_def_node_owner_key(owner_ts.record_def)) {
        if (resolve_from_key(*key)) return owner;
      }
    }

    if (owner_ts.namespace_context_id >= 0 &&
        owner_ts.tag_text_id != kInvalidText) {
      const HirRecordOwnerKey key =
          make_hir_record_owner_key(
              range_for_type_ns_qual(owner_ts), owner_ts.tag_text_id);
      if (resolve_from_key(key)) return owner;
    }

    if (owner_ts.tpl_struct_origin && owner_ts.tpl_struct_origin[0]) {
      if (!ctx.tpl_bindings.empty()) {
        seed_and_resolve_pending_template_type_if_needed(
            owner_ts, ctx.tpl_bindings, ctx.nttp_bindings, n,
            PendingTemplateTypeKind::OwnerStruct, context_name);
      } else {
        TypeBindings empty_tb;
        realize_template_struct_if_needed(owner_ts, empty_tb, ctx.nttp_bindings);
      }
      if (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF) {
        if (const std::optional<HirRecordOwnerKey> key =
                make_struct_def_node_owner_key(owner_ts.record_def)) {
          if (resolve_from_key(*key)) return owner;
        }
      }
      if (owner_ts.namespace_context_id >= 0 &&
          owner_ts.tag_text_id != kInvalidText) {
        const HirRecordOwnerKey key =
            make_hir_record_owner_key(
                range_for_type_ns_qual(owner_ts), owner_ts.tag_text_id);
        if (resolve_from_key(key)) return owner;
      }
    }

    if (!owner.has_structured_identity && module_ && module_->link_name_texts &&
        owner_ts.tag_text_id != kInvalidText) {
      const std::string_view rendered =
          module_->link_name_texts->lookup(owner_ts.tag_text_id);
      if (!rendered.empty() && module_->struct_defs.count(std::string(rendered))) {
        owner.tag = std::string(rendered);
      }
    }
    return owner;
  };

  auto make_method_key =
      [&](const HirRecordOwnerKey& owner_key, const char* method_name,
          bool is_const_method) -> std::optional<HirStructMethodLookupKey> {
    if (!module_ || !module_->link_name_texts || !method_name || !method_name[0]) {
      return std::nullopt;
    }
    const TextId method_text_id = module_->link_name_texts->find(method_name);
    if (method_text_id == kInvalidText) return std::nullopt;
    HirStructMethodLookupKey key;
    key.owner_key = owner_key;
    key.method_text_id = method_text_id;
    key.is_const_method = is_const_method;
    if (!hir_struct_method_lookup_key_has_complete_metadata(key)) {
      return std::nullopt;
    }
    return key;
  };

  auto find_owner_method_mangled =
      [&](const RangeForOwner& owner, const char* method_name,
          bool is_const_obj) -> std::optional<std::string> {
    auto try_const = [&](const HirRecordOwnerKey& key,
                         bool is_const_method) -> std::optional<std::string> {
      const std::optional<HirStructMethodLookupKey> method_key =
          make_method_key(key, method_name, is_const_method);
      if (!method_key) return std::nullopt;
      const auto it = struct_methods_by_owner_.find(*method_key);
      return it == struct_methods_by_owner_.end()
                 ? std::nullopt
                 : std::optional<std::string>(it->second);
    };
    if (owner.owner_key) {
      if (auto local = try_const(*owner.owner_key, is_const_obj)) return local;
      if (auto local = try_const(*owner.owner_key, !is_const_obj)) return local;
    }
    if (!owner.has_structured_identity && !owner.tag.empty()) {
      return find_struct_method_mangled(owner.tag, method_name, is_const_obj);
    }
    if (!owner.tag.empty()) {
      auto dit = module_->struct_defs.find(owner.tag);
      if (dit != module_->struct_defs.end()) {
        for (const auto& base_tag : dit->second.base_tags) {
          if (const std::optional<HirStructMethodLookupKey> base_key =
                  make_struct_method_lookup_key(
                      base_tag, method_name, is_const_obj)) {
            const auto it = struct_methods_by_owner_.find(*base_key);
            if (it != struct_methods_by_owner_.end()) return it->second;
          }
          if (const std::optional<HirStructMethodLookupKey> base_alt_key =
                  make_struct_method_lookup_key(
                      base_tag, method_name, !is_const_obj)) {
            const auto it = struct_methods_by_owner_.find(*base_alt_key);
            if (it != struct_methods_by_owner_.end()) return it->second;
          }
        }
      }
    }
    return std::nullopt;
  };

  auto find_owner_method_return_type =
      [&](const RangeForOwner& owner, const char* method_name,
          bool is_const_obj) -> std::optional<TypeSpec> {
    auto try_const = [&](const HirRecordOwnerKey& key,
                         bool is_const_method) -> std::optional<TypeSpec> {
      const std::optional<HirStructMethodLookupKey> method_key =
          make_method_key(key, method_name, is_const_method);
      if (!method_key) return std::nullopt;
      const auto it = struct_method_ret_types_by_owner_.find(*method_key);
      return it == struct_method_ret_types_by_owner_.end()
                 ? std::nullopt
                 : std::optional<TypeSpec>(it->second);
    };
    if (owner.owner_key) {
      if (auto local = try_const(*owner.owner_key, is_const_obj)) return local;
      if (auto local = try_const(*owner.owner_key, !is_const_obj)) return local;
    }
    if (!owner.has_structured_identity && !owner.tag.empty()) {
      return find_struct_method_return_type(owner.tag, method_name, is_const_obj);
    }
    if (!owner.tag.empty()) {
      auto dit = module_->struct_defs.find(owner.tag);
      if (dit != module_->struct_defs.end()) {
        for (const auto& base_tag : dit->second.base_tags) {
          if (const std::optional<HirStructMethodLookupKey> base_key =
                  make_struct_method_lookup_key(
                      base_tag, method_name, is_const_obj)) {
            const auto it = struct_method_ret_types_by_owner_.find(*base_key);
            if (it != struct_method_ret_types_by_owner_.end()) return it->second;
          }
          if (const std::optional<HirStructMethodLookupKey> base_alt_key =
                  make_struct_method_lookup_key(
                      base_tag, method_name, !is_const_obj)) {
            const auto it = struct_method_ret_types_by_owner_.find(*base_alt_key);
            if (it != struct_method_ret_types_by_owner_.end()) return it->second;
          }
        }
      }
    }
    return std::nullopt;
  };

  TypeSpec range_ts = infer_generic_ctrl_type(&ctx, range_node);
  const RangeForOwner range_owner =
      resolve_structured_owner(range_ts, "range-for-method-owner");
  if (range_ts.base != TB_STRUCT || range_owner.tag.empty()) {
    throw std::runtime_error(
        std::string("error: range-for expression is not a struct type (line ") +
        std::to_string(n->line) + ")");
  }
  const std::string range_error_tag =
      diagnostic_owner_name(range_ts, range_owner);

  auto find_method = [&](const char* method_name) -> std::string {
    auto method = find_owner_method_mangled(
        range_owner, method_name, range_ts.is_const);
    if (!method) {
      throw std::runtime_error(
          std::string("error: range-for: no ") + method_name +
          "() method on struct " + range_error_tag + " (line " +
          std::to_string(n->line) + ")");
    }
    return *method;
  };

  std::string begin_mangled = find_method("begin");
  std::string end_mangled = find_method("end");

  TypeSpec iter_ts{};
  iter_ts.base = TB_VOID;
  {
    DeclRef dr{};
    dr.name = begin_mangled;
    dr.link_name_id = module_->link_names.find(dr.name);
    if (const Function* fn = module_->resolve_range_for_method_callee(dr)) {
      iter_ts = fn->return_type.spec;
    }
  }
  if (iter_ts.base == TB_VOID) {
    if (auto rit =
            find_owner_method_return_type(range_owner, "begin", false)) {
      iter_ts = *rit;
    }
  }
  if (iter_ts.base == TB_VOID) {
    throw std::runtime_error(
        std::string("error: range-for: cannot determine iterator type from begin() (line ") +
        std::to_string(n->line) + ")");
  }

  ExprId range_id = lower_expr(&ctx, range_node);

  auto build_method_call = [&](const std::string& mangled, ExprId obj_id) -> ExprId {
    CallExpr cc{};
    DeclRef dr{};
    dr.name = mangled;
    dr.link_name_id = module_->link_names.find(dr.name);
    TypeSpec callee_ts = iter_ts;
    callee_ts.ptr_level++;
    cc.callee = append_expr(n, dr, callee_ts);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = obj_id;
    TypeSpec ptr_ts = range_ts;
    ptr_ts.ptr_level++;
    cc.args.push_back(append_expr(n, addr, ptr_ts));
    return append_expr(n, cc, iter_ts);
  };

  ExprId begin_call = build_method_call(begin_mangled, range_id);
  ExprId end_call = build_method_call(end_mangled, range_id);

  auto make_iter_local = [&](const char* name, ExprId init_expr) -> LocalId {
    LocalDecl ld{};
    ld.id = next_local_id();
    ld.name = name;
    ld.type = qtype_from(iter_ts, ValueCategory::LValue);
    ld.init = init_expr;
    LocalId lid = ld.id;
    ctx.locals[name] = lid;
    ctx.local_types.insert(lid, iter_ts);
    append_stmt(ctx, Stmt{StmtPayload{std::move(ld)}, make_span(n)});
    return lid;
  };

  LocalId begin_lid = make_iter_local("__range_begin", begin_call);
  LocalId end_lid = make_iter_local("__range_end", end_call);
  const RangeForOwner iter_owner =
      resolve_structured_owner(iter_ts, "range-for-iterator-method-owner");
  const std::string iter_error_tag =
      diagnostic_owner_name(iter_ts, iter_owner);

  auto ref_local = [&](const char* name, LocalId lid) -> ExprId {
    DeclRef dr{};
    dr.name = name;
    dr.local = lid;
    return append_expr(n, dr, iter_ts, ValueCategory::LValue);
  };

  ExprId cond_expr;
  {
    auto method = find_owner_method_mangled(
        iter_owner, "operator_neq", false);
    if (!method) {
      throw std::runtime_error(
          std::string("error: range-for: iterator type ") + iter_error_tag +
          " has no operator!= (line " + std::to_string(n->line) + ")");
    }
    CallExpr cc{};
    DeclRef dr{};
    dr.name = *method;
    dr.link_name_id = module_->link_names.find(dr.name);
    TypeSpec bool_ts{};
    bool_ts.base = TB_BOOL;
    TypeSpec callee_ts = bool_ts;
    callee_ts.ptr_level++;
    cc.callee = append_expr(n, dr, callee_ts);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = ref_local("__range_begin", begin_lid);
    TypeSpec ptr_ts = iter_ts;
    ptr_ts.ptr_level++;
    cc.args.push_back(append_expr(n, addr, ptr_ts));
    cc.args.push_back(ref_local("__range_end", end_lid));
    cond_expr = append_expr(n, cc, bool_ts);
  }

  ExprId update_expr;
  {
    auto method = find_owner_method_mangled(
        iter_owner, "operator_preinc", false);
    if (!method) {
      throw std::runtime_error(
          std::string("error: range-for: iterator type ") + iter_error_tag +
          " has no prefix operator++ (line " + std::to_string(n->line) + ")");
    }
    CallExpr cc{};
    DeclRef dr{};
    dr.name = *method;
    dr.link_name_id = module_->link_names.find(dr.name);
    TypeSpec inc_ret_ts = iter_ts;
    {
      if (const Function* fn = module_->resolve_range_for_method_callee(dr)) {
        inc_ret_ts = fn->return_type.spec;
      }
    }
    TypeSpec callee_ts = inc_ret_ts;
    callee_ts.ptr_level++;
    cc.callee = append_expr(n, dr, callee_ts);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = ref_local("__range_begin", begin_lid);
    TypeSpec ptr_ts = iter_ts;
    ptr_ts.ptr_level++;
    cc.args.push_back(append_expr(n, addr, ptr_ts));
    update_expr = append_expr(n, cc, inc_ret_ts);
  }

  ForStmt fs{};
  fs.cond = cond_expr;
  fs.update = update_expr;
  const BlockId body_b = create_block(ctx);
  const BlockId after_b = create_block(ctx);
  fs.body_block = body_b;
  fs.continue_target = body_b;
  fs.break_target = after_b;
  append_stmt(ctx, Stmt{StmtPayload{fs}, make_span(n)});
  ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

  ctx.break_stack.push_back(after_b);
  ctx.continue_stack.push_back(body_b);
  ctx.current_block = body_b;

  {
    ExprId deref_expr;
    TypeSpec deref_ret_ts{};
    deref_ret_ts.base = TB_INT;
    {
      auto method = find_owner_method_mangled(
          iter_owner, "operator_deref", false);
      if (!method) {
        throw std::runtime_error(
            std::string("error: range-for: iterator type ") + iter_error_tag +
            " has no operator* (line " + std::to_string(n->line) + ")");
      }
      CallExpr cc{};
      DeclRef dr{};
      dr.name = *method;
      dr.link_name_id = module_->link_names.find(dr.name);
      {
        if (const Function* fn = module_->resolve_range_for_method_callee(dr)) {
          deref_ret_ts = fn->return_type.spec;
        }
      }
      if (deref_ret_ts.base == TB_VOID || deref_ret_ts.base == TB_INT) {
        if (auto rit = find_owner_method_return_type(
                iter_owner, "operator_deref", false)) {
          deref_ret_ts = *rit;
        }
      }
      TypeSpec callee_ts = deref_ret_ts;
      callee_ts.ptr_level++;
      cc.callee = append_expr(n, dr, callee_ts);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = ref_local("__range_begin", begin_lid);
      TypeSpec ptr_ts = iter_ts;
      ptr_ts.ptr_level++;
      cc.args.push_back(append_expr(n, addr, ptr_ts));
      deref_expr = append_expr(n, cc, reference_storage_ts(deref_ret_ts));
    }

    if (decl_node) {
      LocalDecl ld{};
      ld.id = next_local_id();
      ld.name = decl_node->name ? decl_node->name : "__range_var";
      TypeSpec var_ts = decl_node->type;
      bool is_ref = var_ts.is_lvalue_ref;
      if (var_ts.base == TB_AUTO) {
        bool was_const = var_ts.is_const;
        var_ts = deref_ret_ts;
        var_ts.is_lvalue_ref = false;
        if (was_const) var_ts.is_const = true;
        if (is_ref) var_ts.is_lvalue_ref = true;
      }
      resolve_typedef_to_struct(var_ts);
      ld.type = qtype_from(reference_storage_ts(var_ts), ValueCategory::LValue);
      if (is_ref) {
        if (deref_ret_ts.is_lvalue_ref) {
          ld.init = deref_expr;
        } else {
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = deref_expr;
          ld.init = append_expr(n, addr, ld.type.spec);
        }
      } else {
        ld.init = deref_expr;
      }
      ctx.locals[ld.name] = ld.id;
      ctx.local_types.insert(ld.id, var_ts);
      append_stmt(ctx, Stmt{StmtPayload{std::move(ld)}, make_span(n)});
    }
  }

  if (n->body) lower_stmt_node(ctx, n->body);

  if (ctx.current_block.value != after_b.value &&
      !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
    ContinueStmt cont{};
    cont.target = body_b;
    append_stmt(ctx, Stmt{StmtPayload{cont}, make_span(n)});
    ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
  }
  ctx.break_stack.pop_back();
  ctx.continue_stack.pop_back();
  ctx.current_block = after_b;
}

}  // namespace c4c::hir
