// Draft-only HIR expression lowering split extracted from ast_to_hir.cpp.
// This file is not yet wired into the build and is intended as a staging
// artifact for the eventual multi-translation-unit split.

#include "ast_to_hir.hpp"
#include "hir_lowerer_internal.hpp"
#include "consteval.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>
#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "type_utils.hpp"
#include "../parser/parser_internal.hpp"

namespace c4c::hir {

namespace {

class LayoutQueries {
 public:
  explicit LayoutQueries(const hir::Module& module) : module_(module) {}

  int struct_size_bytes(const char* tag) const {
    if (!tag || !tag[0]) return 4;
    const auto it = module_.struct_defs.find(tag);
    if (it == module_.struct_defs.end()) return 4;
    return it->second.size_bytes;
  }

  int struct_align_bytes(const char* tag) const {
    if (!tag || !tag[0]) return 4;
    const auto it = module_.struct_defs.find(tag);
    if (it == module_.struct_defs.end()) return 4;
    return std::max(1, it->second.align_bytes);
  }

  int type_size_bytes(const TypeSpec& ts) const {
    if (ts.array_rank > 0) {
      if (ts.array_size == 0) return 0;
      if (ts.array_size > 0) {
        const TypeSpec elem = array_element_type(ts);
        return static_cast<int>(ts.array_size) * type_size_bytes(elem);
      }
      return 8;
    }
    if (ts.is_vector && ts.vector_bytes > 0) return static_cast<int>(ts.vector_bytes);
    if (ts.ptr_level > 0 && ts.is_ptr_to_array) return 8;
    if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
    if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
      return struct_size_bytes(ts.tag);
    }
    return sizeof_base(ts.base);
  }

  int type_align_bytes(const TypeSpec& ts) const {
    int natural = 1;
    if (ts.array_rank > 0) {
      natural = type_align_bytes(array_element_type(ts));
    } else if (ts.is_vector && ts.vector_bytes > 0) {
      natural = static_cast<int>(ts.vector_bytes);
    } else if (ts.ptr_level > 0 || ts.is_fn_ptr) {
      natural = 8;
    } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
      natural = struct_align_bytes(ts.tag);
    } else {
      natural = std::max(1, static_cast<int>(align_base(ts.base, ts.ptr_level)));
    }
    if (ts.align_bytes > 0) natural = std::max(natural, ts.align_bytes);
    return natural;
  }

 private:
  static TypeSpec array_element_type(const TypeSpec& ts) {
    TypeSpec elem = ts;
    elem.array_rank--;
    if (elem.array_rank > 0) {
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
    }
    elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
    return elem;
  }

  const hir::Module& module_;
};

}  // namespace

ExprId Lowerer::hoist_compound_literal_to_global(const Node* addr_node,
                                                 const Node* clit) {
  GlobalVar cg{};
  cg.id = next_global_id();
  const std::string clit_name = "__clit_" + std::to_string(cg.id.value);
  cg.name = clit_name;
  cg.type = qtype_from(clit->type, ValueCategory::LValue);
  cg.linkage = {true, false, false};
  cg.is_const = false;
  cg.span = make_span(clit);
  if (clit->left && clit->left->kind == NK_INIT_LIST) {
    cg.init = lower_init_list(clit->left);
    cg.type.spec = resolve_array_ts(cg.type.spec, cg.init);
    cg.init = normalize_global_init(cg.type.spec, cg.init);
  }
  module_->global_index[clit_name] = cg.id;
  module_->globals.push_back(std::move(cg));

  TypeSpec ptr_ts = clit->type;
  ptr_ts.ptr_level++;
  DeclRef dr{};
  dr.name = clit_name;
  dr.global = module_->global_index[clit_name];
  ExprId dr_id = append_expr(clit, dr, clit->type, ValueCategory::LValue);
  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = dr_id;
  return append_expr(addr_node, addr, ptr_ts);
}

bool Lowerer::is_ast_lvalue(const Node* n) {
  if (!n) return false;
  switch (n->kind) {
    case NK_VAR:
    case NK_INDEX:
    case NK_DEREF:
    case NK_MEMBER:
      return true;
    default:
      return false;
  }
}

ExprId Lowerer::lower_call_expr(FunctionCtx* ctx, const Node* n) {
  if (auto consteval_expr = try_lower_consteval_call_expr(ctx, n)) {
    return *consteval_expr;
  }

  if (ctx && n->left && n->left->kind == NK_VAR && n->left->name &&
      n->n_children == 0) {
    auto sit = module_->struct_defs.find(n->left->name);
    auto cit = struct_constructors_.find(n->left->name);
    if (sit != module_->struct_defs.end() &&
        (cit == struct_constructors_.end() || cit->second.empty())) {
      TypeSpec tmp_ts{};
      tmp_ts.base = TB_STRUCT;
      tmp_ts.tag = sit->second.tag.c_str();
      tmp_ts.array_size = -1;
      tmp_ts.inner_rank = -1;

      const LocalId tmp_lid = next_local_id();
      const std::string tmp_name = "__brace_tmp_" + std::to_string(tmp_lid.value);
      LocalDecl tmp{};
      tmp.id = tmp_lid;
      tmp.name = tmp_name;
      tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
      tmp.storage = StorageClass::Auto;
      tmp.span = make_span(n);
      ctx->locals[tmp.name] = tmp.id;
      ctx->local_types[tmp.id.value] = tmp_ts;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

      DeclRef tmp_ref{};
      tmp_ref.name = tmp_name;
      tmp_ref.local = tmp_lid;
      return append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
    }
  }

  if (ctx && n->left && n->left->kind == NK_VAR && n->left->name) {
    const std::string callee_name = n->left->name;
    auto cit = struct_constructors_.find(callee_name);
    auto sit = module_->struct_defs.find(callee_name);
    if (cit != struct_constructors_.end() && sit != module_->struct_defs.end()) {
      const CtorOverload* best = nullptr;
      if (cit->second.size() == 1) {
        if (cit->second[0].method_node->n_params == n->n_children)
          best = &cit->second[0];
      } else {
        int best_score = -1;
        for (const auto& ov : cit->second) {
          if (ov.method_node->n_params != n->n_children) continue;
          int score = 0;
          bool viable = true;
          for (int pi = 0; pi < ov.method_node->n_params && viable; ++pi) {
            TypeSpec param_ts = ov.method_node->params[pi]->type;
            resolve_typedef_to_struct(param_ts);
            TypeSpec arg_ts = infer_generic_ctrl_type(ctx, n->children[pi]);
            const bool arg_is_lvalue = is_ast_lvalue(n->children[pi]);
            TypeSpec param_base = param_ts;
            param_base.is_lvalue_ref = false;
            param_base.is_rvalue_ref = false;
            if (param_base.base == arg_ts.base &&
                param_base.ptr_level == arg_ts.ptr_level) {
              score += 2;
              if (param_ts.is_rvalue_ref && !arg_is_lvalue) {
                score += 4;
              } else if (param_ts.is_rvalue_ref && arg_is_lvalue) {
                viable = false;
              } else if (param_ts.is_lvalue_ref && arg_is_lvalue) {
                score += 3;
              } else if (param_ts.is_lvalue_ref && !arg_is_lvalue) {
                score += 1;
              }
            } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                       param_base.base != TB_STRUCT && arg_ts.base != TB_STRUCT &&
                       param_base.base != TB_VOID && arg_ts.base != TB_VOID) {
              score += 1;
            } else {
              viable = false;
            }
          }
          if (viable && score > best_score) {
            best_score = score;
            best = &ov;
          }
        }
      }

      if (best) {
        if (best->method_node->is_deleted) {
          std::string diag = "error: call to deleted constructor '";
          diag += callee_name;
          diag += "'";
          throw std::runtime_error(diag);
        }

        TypeSpec tmp_ts{};
        tmp_ts.base = TB_STRUCT;
        tmp_ts.tag = sit->second.tag.c_str();
        tmp_ts.array_size = -1;
        tmp_ts.inner_rank = -1;

        const LocalId tmp_lid = next_local_id();
        const std::string tmp_name = "__ctor_tmp_" + std::to_string(tmp_lid.value);
        LocalDecl tmp{};
        tmp.id = tmp_lid;
        tmp.name = tmp_name;
        tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
        tmp.storage = StorageClass::Auto;
        tmp.span = make_span(n);
        ctx->locals[tmp.name] = tmp.id;
        ctx->local_types[tmp.id.value] = tmp_ts;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

        CallExpr ctor_call{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        ctor_call.callee = append_expr(n, callee_ref, callee_ts);

        DeclRef tmp_ref{};
        tmp_ref.name = tmp_name;
        tmp_ref.local = tmp_lid;
        ExprId tmp_id = append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = tmp_id;
        TypeSpec ptr_ts = tmp_ts;
        ptr_ts.ptr_level++;
        ctor_call.args.push_back(append_expr(n, addr, ptr_ts));

        for (int i = 0; i < n->n_children; ++i) {
          TypeSpec param_ts = best->method_node->params[i]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
            const Node* arg = n->children[i];
            const Node* inner = arg;
            if (inner->kind == NK_CAST && inner->left) inner = inner->left;
            ExprId arg_val = lower_expr(ctx, inner);
            TypeSpec storage_ts = reference_storage_ts(param_ts);
            UnaryExpr addr_e{};
            addr_e.op = UnaryOp::AddrOf;
            addr_e.operand = arg_val;
            ctor_call.args.push_back(append_expr(arg, addr_e, storage_ts));
          } else {
            ctor_call.args.push_back(lower_expr(ctx, n->children[i]));
          }
        }

        ExprId call_id = append_expr(n, ctor_call, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
        return tmp_id;
      }
    }
  }

  if (ctx && n->left && n->left->kind == NK_VAR && n->left->name &&
      n->left->has_template_args &&
      find_template_struct_primary(n->left->name)) {
    ExprId tmp_expr = lower_expr(ctx, n->left);
    if (n->n_children == 0) return tmp_expr;
    const TypeSpec& tmp_ts = module_->expr_pool[tmp_expr.value].type.spec;
    if (tmp_ts.base == TB_STRUCT && tmp_ts.ptr_level == 0 && tmp_ts.tag) {
      auto resolved = find_struct_method_mangled(tmp_ts.tag, "operator_call", false);
      if (!resolved)
        resolved = find_struct_method_mangled(tmp_ts.tag, "operator_call", true);
      if (resolved) {
        ExprId this_obj = tmp_expr;
        if (module_->expr_pool[tmp_expr.value].type.category != ValueCategory::LValue) {
          LocalDecl tmp{};
          tmp.id = next_local_id();
          std::string tmp_name = "__call_tmp_" + std::to_string(tmp.id.value);
          tmp.name = tmp_name;
          tmp.type = qtype_from(tmp_ts, ValueCategory::RValue);
          const TypeSpec init_ts = module_->expr_pool[tmp_expr.value].type.spec;
          if (init_ts.base == tmp_ts.base && init_ts.ptr_level == tmp_ts.ptr_level &&
              init_ts.tag == tmp_ts.tag) {
            tmp.init = tmp_expr;
          }
          append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

          DeclRef tmp_ref{};
          tmp_ref.name = tmp_name;
          tmp_ref.local = LocalId{tmp.id.value};
          this_obj = append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
        }
        std::string resolved_mangled = *resolved;
        CallExpr oc{};
        DeclRef dr{};
        dr.name = resolved_mangled;
        auto fit = module_->fn_index.find(dr.name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        if (fit != module_->fn_index.end() &&
            fit->second.value < module_->functions.size())
          fn_ts = module_->functions[fit->second.value].return_type.spec;
        if (fn_ts.base == TB_VOID) {
          if (auto rit = find_struct_method_return_type(
                  tmp_ts.tag, "operator_call", false))
            fn_ts = *rit;
        }
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        oc.callee = append_expr(n, dr, callee_ts);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = this_obj;
        TypeSpec ptr_ts = tmp_ts;
        ptr_ts.ptr_level++;
        oc.args.push_back(append_expr(n, addr, ptr_ts));
        for (int i = 0; i < n->n_children; ++i)
          oc.args.push_back(lower_expr(ctx, n->children[i]));
        return append_expr(n, oc, fn_ts);
      }
    }
    return tmp_expr;
  }

  if (n->left) {
    TypeSpec callee_ts = infer_generic_ctrl_type(ctx, n->left);
    if (callee_ts.base == TB_STRUCT && callee_ts.ptr_level == 0 && callee_ts.tag) {
      std::vector<const Node*> arg_nodes;
      for (int i = 0; i < n->n_children; ++i)
        arg_nodes.push_back(n->children[i]);
      ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_call", arg_nodes);
      if (op.valid()) return op;
    }
  }

  CallExpr c{};
  auto maybe_lower_ref_arg = [&](const Node* arg_node, const TypeSpec* param_ts) -> ExprId {
    if (ctx && arg_node && arg_node->kind == NK_INIT_LIST && param_ts &&
        !param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref) {
      TypeSpec direct_ts = *param_ts;
      direct_ts.is_lvalue_ref = false;
      direct_ts.is_rvalue_ref = false;
      resolve_typedef_to_struct(direct_ts);
      if (direct_ts.base == TB_STRUCT && direct_ts.ptr_level == 0) {
        if (describe_initializer_list_struct(direct_ts, nullptr, nullptr, nullptr))
          return materialize_initializer_list_arg(ctx, arg_node, direct_ts);
      }
    }
    if (!param_ts || (!param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref))
      return lower_expr(ctx, arg_node);
    TypeSpec storage_ts = reference_storage_ts(*param_ts);
    if (param_ts->is_rvalue_ref) {
      if (arg_node && arg_node->kind == NK_CAST && arg_node->type.is_rvalue_ref &&
          arg_node->left) {
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = lower_expr(ctx, arg_node->left);
        return append_expr(arg_node, addr, storage_ts);
      }
      ExprId arg_val = lower_expr(ctx, arg_node);
      TypeSpec val_ts = reference_value_ts(*param_ts);
      resolve_typedef_to_struct(val_ts);
      LocalDecl tmp{};
      tmp.id = next_local_id();
      tmp.name = "__rref_arg_tmp";
      tmp.type = qtype_from(val_ts, ValueCategory::LValue);
      tmp.init = arg_val;
      const LocalId tmp_lid = tmp.id;
      ctx->locals[tmp.name] = tmp.id;
      ctx->local_types[tmp.id.value] = val_ts;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(arg_node)});
      DeclRef tmp_ref{};
      tmp_ref.name = "__rref_arg_tmp";
      tmp_ref.local = tmp_lid;
      ExprId var_id = append_expr(arg_node, tmp_ref, val_ts, ValueCategory::LValue);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = var_id;
      return append_expr(arg_node, addr, storage_ts);
    }
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = lower_expr(ctx, arg_node);
    return append_expr(arg_node, addr, storage_ts);
  };
  auto direct_callee_fn = [&](const std::string& name) -> const Function* {
    auto fit = module_->fn_index.find(name);
    if (fit == module_->fn_index.end()) return nullptr;
    return module_->find_function(fit->second);
  };
  auto try_expand_pack_call_arg =
      [&](const Node* arg_node, const TypeSpec* construct_param_ts) -> bool {
    if (!ctx || !arg_node || arg_node->kind != NK_PACK_EXPANSION || !arg_node->left)
      return false;
    const Node* pattern = arg_node->left;
    if (pattern->kind != NK_CALL || pattern->n_children != 1 || !pattern->left)
      return false;
    if (pattern->left->kind != NK_VAR || !pattern->left->name ||
        std::strcmp(pattern->left->name, "forward") != 0)
      return false;
    if (pattern->left->n_template_args != 1 || !pattern->left->template_arg_types ||
        pattern->left->template_arg_types[0].base != TB_TYPEDEF ||
        !pattern->left->template_arg_types[0].tag)
      return false;
    const Node* forwarded_arg = pattern->children[0];
    if (!forwarded_arg || forwarded_arg->kind != NK_VAR || !forwarded_arg->name)
      return false;
    auto pit = ctx->pack_params.find(forwarded_arg->name);
    if (pit == ctx->pack_params.end() || pit->second.empty()) return false;

    for (const auto& elem : pit->second) {
      const Node* tpl_fn = ct_state_->find_template_def("forward");
      TypeBindings forward_bindings;
      if (tpl_fn && tpl_fn->n_template_params > 0 && tpl_fn->template_param_names[0])
        forward_bindings[tpl_fn->template_param_names[0]] = elem.type;
      else
        forward_bindings["T"] = elem.type;
      const std::string callee_name =
          mangle_template_name("forward", forward_bindings);

      DeclRef callee_ref{};
      callee_ref.name = callee_name;
      TypeSpec callee_ts = pattern->left->type;
      if (const Function* callee_fn = direct_callee_fn(callee_name)) {
        callee_ts = callee_fn->return_type.spec;
        callee_ts.ptr_level++;
      }

      CallExpr expanded_call{};
      expanded_call.callee = append_expr(pattern->left, callee_ref, callee_ts);
      TemplateCallInfo tci;
      tci.source_template = "forward";
      tci.template_args.push_back(elem.type);
      expanded_call.template_info = std::move(tci);

      DeclRef param_ref{};
      param_ref.name = elem.element_name;
      param_ref.param_index = elem.param_index;
      ExprId param_expr = append_expr(forwarded_arg, param_ref, elem.type, ValueCategory::LValue);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = param_expr;
      TypeSpec param_storage_ts = elem.type;
      param_storage_ts.ptr_level += 1;
      expanded_call.args.push_back(append_expr(forwarded_arg, addr, param_storage_ts));

      TypeSpec ret_ts = elem.type;
      ret_ts.is_lvalue_ref = false;
      ret_ts.is_rvalue_ref = true;
      ExprId expanded_id = append_expr(pattern, expanded_call, ret_ts);
      if (!construct_param_ts || (!construct_param_ts->is_lvalue_ref &&
                                  !construct_param_ts->is_rvalue_ref)) {
        c.args.push_back(expanded_id);
      } else {
        TypeSpec storage_ts = reference_storage_ts(*construct_param_ts);
        UnaryExpr outer_addr{};
        outer_addr.op = UnaryOp::AddrOf;
        outer_addr.operand = expanded_id;
        c.args.push_back(append_expr(pattern, outer_addr, storage_ts));
      }
    }
    return true;
  };
  if (n->left && n->left->kind == NK_MEMBER && n->left->name) {
    const Node* base_node = n->left->left;
    const char* method_name = n->left->name;
    TypeSpec base_ts = infer_generic_ctrl_type(ctx, base_node);
    if (n->left->is_arrow && base_ts.ptr_level > 0)
      base_ts.ptr_level--;
    const char* tag = base_ts.tag;
    if (tag) {
      std::string base_key = std::string(tag) + "::" + method_name;
      std::string const_key = base_key + "_const";
      decltype(struct_methods_)::iterator mit;
      if (base_ts.is_const) {
        mit = struct_methods_.find(const_key);
        if (mit == struct_methods_.end())
          mit = struct_methods_.find(base_key);
      } else {
        mit = struct_methods_.find(base_key);
        if (mit == struct_methods_.end())
          mit = struct_methods_.find(const_key);
      }
      if (mit != struct_methods_.end()) {
        DeclRef dr{};
        std::string resolved_method = mit->second;
        auto ovit = ref_overload_set_.find(mit->first);
        if (ovit != ref_overload_set_.end() && !ovit->second.empty()) {
          resolved_method = resolve_ref_overload(mit->first, n);
        }
        dr.name = resolved_method;
        auto fit = module_->fn_index.find(dr.name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        if (fit != module_->fn_index.end() &&
            fit->second.value < module_->functions.size()) {
          fn_ts = module_->functions[fit->second.value].return_type.spec;
        }
        fn_ts.ptr_level++;
        c.callee = append_expr(n->left, dr, fn_ts);
        ExprId base_id = lower_expr(ctx, base_node);
        if (n->left->is_arrow) {
          c.args.push_back(base_id);
        } else {
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = base_id;
          TypeSpec ptr_ts = base_ts;
          ptr_ts.ptr_level++;
          c.args.push_back(append_expr(base_node, addr, ptr_ts));
        }
        const Node* callee_method = find_pending_method_by_mangled(resolved_method);
        for (int i = 0; i < n->n_children; ++i) {
          const TypeSpec* param_ts = nullptr;
          if (callee_method && i < callee_method->n_params &&
              callee_method->params[i]) {
            param_ts = &callee_method->params[i]->type;
          }
          if (try_expand_pack_call_arg(n->children[i], param_ts)) continue;
          c.args.push_back(maybe_lower_ref_arg(n->children[i], param_ts));
        }
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID && ts.ptr_level == 0) {
          auto mfit = module_->fn_index.find(resolved_method);
          if (mfit != module_->fn_index.end() &&
              mfit->second.value < module_->functions.size())
            ts = module_->functions[mfit->second.value].return_type.spec;
        }
        return append_expr(n, c, ts);
      }
    }
  }

  std::string resolved_callee_name;
  if (n->left && n->left->kind == NK_VAR && n->left->name &&
      (n->left->n_template_args > 0 || n->left->has_template_args) &&
      ct_state_->has_template_def(n->left->name) &&
      !ct_state_->has_consteval_def(n->left->name)) {
    const TypeBindings* enc = (ctx && !ctx->tpl_bindings.empty())
                                  ? &ctx->tpl_bindings : nullptr;
    const NttpBindings* enc_nttp = (ctx && !ctx->nttp_bindings.empty())
                                       ? &ctx->nttp_bindings : nullptr;
    const Node* tpl_fn = ct_state_->find_template_def(n->left->name);
    TypeBindings bindings;
    NttpBindings nttp_bindings;
    if (auto ded_it = deduced_template_calls_.find(n);
        ded_it != deduced_template_calls_.end()) {
      resolved_callee_name = ded_it->second.mangled_name;
      bindings = ded_it->second.bindings;
      nttp_bindings = ded_it->second.nttp_bindings;
    } else {
      bindings = merge_explicit_and_deduced_type_bindings(n, n->left, tpl_fn, enc);
      nttp_bindings = build_call_nttp_bindings(n->left, tpl_fn, enc_nttp);
      resolved_callee_name =
          mangle_template_name(n->left->name, bindings, nttp_bindings);
      if (tpl_fn) {
        const auto param_order = get_template_param_order(tpl_fn, &bindings, &nttp_bindings);
        const SpecializationKey spec_key = nttp_bindings.empty()
            ? make_specialization_key(n->left->name, param_order, bindings)
            : make_specialization_key(n->left->name, param_order, bindings, nttp_bindings);
        if (const auto* inst_list = registry_.find_instances(n->left->name)) {
          for (const auto& inst : *inst_list) {
            if (inst.spec_key == spec_key) {
              resolved_callee_name = inst.mangled_name;
              break;
            }
          }
        }
      }
    }
    DeclRef dr{};
    dr.name = resolved_callee_name;
    auto fit = module_->fn_index.find(dr.name);
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      TypeSpec fn_ts = module_->functions[fit->second.value].return_type.spec;
      fn_ts.ptr_level++;
      c.callee = append_expr(n->left, dr, fn_ts);
    } else {
      c.callee = append_expr(n->left, dr, n->left->type);
    }
    TemplateCallInfo tci;
    tci.source_template = n->left->name;
    if (tpl_fn) {
      for (const auto& pname : get_template_param_order(tpl_fn, &bindings, &nttp_bindings)) {
        auto bit = bindings.find(pname);
        if (bit != bindings.end()) tci.template_args.push_back(bit->second);
      }
      tci.nttp_args = std::move(nttp_bindings);
    }
    c.template_info = std::move(tci);
  } else if (auto ded_it = deduced_template_calls_.find(n);
             ded_it != deduced_template_calls_.end()) {
    resolved_callee_name = ded_it->second.mangled_name;
    DeclRef dr{};
    dr.name = resolved_callee_name;
    auto fit = module_->fn_index.find(dr.name);
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      TypeSpec fn_ts = module_->functions[fit->second.value].return_type.spec;
      fn_ts.ptr_level++;
      c.callee = append_expr(n->left, dr, fn_ts);
    } else {
      c.callee = append_expr(n->left, dr, n->left->type);
    }
    TemplateCallInfo tci;
    tci.source_template = n->left->name;
    const Node* tpl_fn = ct_state_->find_template_def(n->left->name);
    if (tpl_fn) {
      for (const auto& pname : get_template_param_order(
               tpl_fn, &ded_it->second.bindings, &ded_it->second.nttp_bindings)) {
        auto bit = ded_it->second.bindings.find(pname);
        if (bit != ded_it->second.bindings.end())
          tci.template_args.push_back(bit->second);
      }
      tci.nttp_args = ded_it->second.nttp_bindings;
    }
    c.template_info = std::move(tci);
  } else {
    if (n->left && n->left->kind == NK_VAR && n->left->name &&
        ref_overload_set_.count(n->left->name)) {
      resolved_callee_name = resolve_ref_overload(n->left->name, n);
      DeclRef dr{};
      dr.name = resolved_callee_name;
      auto fit = module_->fn_index.find(dr.name);
      if (fit != module_->fn_index.end() &&
          fit->second.value < module_->functions.size()) {
        TypeSpec fn_ts = module_->functions[fit->second.value].return_type.spec;
        fn_ts.ptr_level++;
        c.callee = append_expr(n->left, dr, fn_ts);
      } else {
        c.callee = append_expr(n->left, dr, n->left->type);
      }
    } else {
      bool resolved_as_method = false;
      if (ctx && !ctx->method_struct_tag.empty() &&
          n->left && n->left->kind == NK_VAR && n->left->name) {
        const std::string callee_name = n->left->name;
        if (!direct_callee_fn(callee_name)) {
          std::string base_key = ctx->method_struct_tag + "::" + callee_name;
          std::string const_key = base_key + "_const";
          auto mit = struct_methods_.find(base_key);
          if (mit == struct_methods_.end())
            mit = struct_methods_.find(const_key);
          if (mit != struct_methods_.end()) {
            resolved_callee_name = mit->second;
            DeclRef dr{};
            dr.name = resolved_callee_name;
            auto fit = module_->fn_index.find(dr.name);
            TypeSpec fn_ts{};
            fn_ts.base = TB_VOID;
            if (fit != module_->fn_index.end() &&
                fit->second.value < module_->functions.size()) {
              fn_ts = module_->functions[fit->second.value].return_type.spec;
            }
            fn_ts.ptr_level++;
            c.callee = append_expr(n->left, dr, fn_ts);
            auto pit = ctx->params.find("this");
            if (pit != ctx->params.end()) {
              DeclRef this_ref{};
              this_ref.name = "this";
              this_ref.param_index = pit->second;
              TypeSpec this_ts{};
              this_ts.base = TB_STRUCT;
              auto sit = module_->struct_defs.find(ctx->method_struct_tag);
              this_ts.tag = sit != module_->struct_defs.end()
                                ? sit->second.tag.c_str()
                                : ctx->method_struct_tag.c_str();
              this_ts.ptr_level = 1;
              c.args.push_back(append_expr(n->left, this_ref, this_ts, ValueCategory::LValue));
            }
            resolved_as_method = true;
          }
        }
      }
      if (!resolved_as_method)
        c.callee = lower_expr(ctx, n->left);
    }
  }
  c.builtin_id = n->builtin_id;
  const Function* callee_fn = !resolved_callee_name.empty()
                                  ? direct_callee_fn(resolved_callee_name)
                                  : (n->left && n->left->kind == NK_VAR && n->left->name
                                         ? direct_callee_fn(n->left->name)
                                         : nullptr);
  for (int i = 0; i < n->n_children; ++i) {
    const TypeSpec* param_ts =
        (callee_fn && static_cast<size_t>(i) < callee_fn->params.size())
            ? &callee_fn->params[static_cast<size_t>(i)].type.spec
            : nullptr;
    c.args.push_back(maybe_lower_ref_arg(n->children[i], param_ts));
  }
  TypeSpec ts = n->type;
  if (n->builtin_id != BuiltinId::Unknown) {
    bool known = false;
    TypeSpec builtin_ts = classify_known_builtin_return_type(n->builtin_id, &known);
    if (known) ts = builtin_ts;
  } else if (auto inferred = infer_call_result_type_from_callee(ctx, n->left)) {
    ts = *inferred;
  } else if (n->left && n->left->kind == NK_VAR && n->left->name) {
    bool known = false;
    TypeSpec kts = classify_known_call_return_type(n->left->name, &known);
    if (known) ts = kts;
  }
  return append_expr(n, c, ts);
}

std::optional<ExprId> Lowerer::try_lower_consteval_call_expr(FunctionCtx* ctx,
                                                             const Node* n) {
  if (!(n->kind == NK_CALL && n->left && n->left->kind == NK_VAR && n->left->name))
    return std::nullopt;
  const Node* ce_fn_def = ct_state_->find_consteval_def(n->left->name);
  if (!ce_fn_def) return std::nullopt;

  ConstEvalEnv arg_env{&enum_consts_, &const_int_bindings_,
                       ctx ? &ctx->local_const_bindings : nullptr};
  TypeBindings tpl_bindings;
  NttpBindings ce_nttp_bindings;
  const Node* fn_def = ce_fn_def;
  if ((n->left->n_template_args > 0 || n->left->has_template_args) &&
      fn_def->n_template_params > 0) {
    int count = std::min(n->left->n_template_args, fn_def->n_template_params);
    for (int i = 0; i < count; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) {
        if (n->left->template_arg_is_value && n->left->template_arg_is_value[i]) {
          if (n->left->template_arg_nttp_names && n->left->template_arg_nttp_names[i] &&
              ctx && !ctx->nttp_bindings.empty()) {
            auto it = ctx->nttp_bindings.find(n->left->template_arg_nttp_names[i]);
            if (it != ctx->nttp_bindings.end()) {
              ce_nttp_bindings[fn_def->template_param_names[i]] = it->second;
            }
          } else {
            ce_nttp_bindings[fn_def->template_param_names[i]] =
                n->left->template_arg_values[i];
          }
        }
        continue;
      }
      TypeSpec arg_ts = n->left->template_arg_types[i];
      if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && ctx && !ctx->tpl_bindings.empty()) {
        auto resolved = ctx->tpl_bindings.find(arg_ts.tag);
        if (resolved != ctx->tpl_bindings.end()) arg_ts = resolved->second;
      }
      tpl_bindings[fn_def->template_param_names[i]] = arg_ts;
    }
    if (fn_def->template_param_has_default) {
      for (int i = count; i < fn_def->n_template_params; ++i) {
        if (!fn_def->template_param_names[i]) continue;
        if (!fn_def->template_param_has_default[i]) continue;
        if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) {
          ce_nttp_bindings[fn_def->template_param_names[i]] =
              fn_def->template_param_default_values[i];
        } else {
          tpl_bindings[fn_def->template_param_names[i]] =
              fn_def->template_param_default_types[i];
        }
      }
    }
    arg_env.type_bindings = &tpl_bindings;
    if (!ce_nttp_bindings.empty()) arg_env.nttp_bindings = &ce_nttp_bindings;
  }
  std::vector<ConstValue> args;
  bool all_const = true;
  for (int i = 0; i < n->n_children; ++i) {
    auto r = evaluate_constant_expr(n->children[i], arg_env);
    if (r.ok()) {
      args.push_back(*r.value);
    } else {
      all_const = false;
      break;
    }
  }
  if (!all_const) {
    std::string diag = "error: call to consteval function '";
    diag += n->left->name;
    diag += "' with non-constant arguments";
    throw std::runtime_error(diag);
  }

  PendingConstevalExpr pce;
  pce.fn_name = n->left->name;
  for (const auto& cv : args) pce.const_args.push_back(cv.as_int());
  pce.tpl_bindings = tpl_bindings;
  pce.nttp_bindings = ce_nttp_bindings;
  pce.call_span = make_span(n);
  pce.unlocked_by_deferred_instantiation = lowering_deferred_instantiation_;
  TypeSpec ts = n->type;
  return append_expr(n, std::move(pce), ts);
}

std::string Lowerer::resolve_ref_overload(const std::string& base_name,
                                          const Node* call_node) {
  auto ovit = ref_overload_set_.find(base_name);
  if (ovit == ref_overload_set_.end()) return {};
  const auto& overloads = ovit->second;
  const std::string* best_name = nullptr;
  int best_score = -1;
  for (const auto& ov : overloads) {
    bool viable = true;
    int score = 0;
    for (int i = 0;
         i < call_node->n_children &&
         i < static_cast<int>(ov.param_is_rvalue_ref.size());
         ++i) {
      bool arg_is_lvalue = is_ast_lvalue(call_node->children[i]);
      if (ov.param_is_lvalue_ref[static_cast<size_t>(i)] && !arg_is_lvalue) {
        viable = false;
        break;
      }
      if (ov.param_is_rvalue_ref[static_cast<size_t>(i)] && arg_is_lvalue) {
        viable = false;
        break;
      }
      if (ov.param_is_rvalue_ref[static_cast<size_t>(i)] && !arg_is_lvalue) {
        score += 2;
      } else if (ov.param_is_lvalue_ref[static_cast<size_t>(i)] && arg_is_lvalue) {
        score += 2;
      } else {
        score += 1;
      }
    }
    if (viable && score > best_score) {
      best_name = &ov.mangled_name;
      best_score = score;
    }
  }
  return best_name ? *best_name : base_name;
}

const Node* Lowerer::find_pending_method_by_mangled(
    const std::string& mangled_name) const {
  for (const auto& pm : pending_methods_) {
    if (pm.mangled == mangled_name) return pm.method_node;
  }
  return nullptr;
}

bool Lowerer::describe_initializer_list_struct(const TypeSpec& ts,
                                               TypeSpec* elem_ts,
                                               TypeSpec* data_ptr_ts,
                                               TypeSpec* len_ts) const {
  if (ts.base != TB_STRUCT || ts.ptr_level != 0 || !ts.tag) return false;
  auto sit = module_->struct_defs.find(ts.tag);
  if (sit == module_->struct_defs.end()) return false;

  const HirStructField* data_field = nullptr;
  const HirStructField* len_field = nullptr;
  for (const auto& field : sit->second.fields) {
    if (field.name == "_M_array") {
      data_field = &field;
    } else if (field.name == "_M_len") {
      len_field = &field;
    }
  }
  if (!data_field || !len_field) return false;
  if (data_field->elem_type.ptr_level <= 0) return false;

  if (data_ptr_ts) *data_ptr_ts = data_field->elem_type;
  if (elem_ts) {
    *elem_ts = data_field->elem_type;
    elem_ts->ptr_level--;
  }
  if (len_ts) *len_ts = len_field->elem_type;
  return true;
}

ExprId Lowerer::materialize_initializer_list_arg(FunctionCtx* ctx,
                                                 const Node* list_node,
                                                 const TypeSpec& param_ts) {
  TypeSpec elem_ts{};
  TypeSpec data_ptr_ts{};
  TypeSpec len_ts{};
  if (!describe_initializer_list_struct(param_ts, &elem_ts, &data_ptr_ts, &len_ts)) {
    return append_expr(list_node, IntLiteral{0, false}, param_ts);
  }

  ExprId data_ptr_id{};
  if (list_node && list_node->n_children > 0) {
    TypeSpec array_ts = elem_ts;
    array_ts.array_rank = 1;
    array_ts.array_size = list_node->n_children;
    array_ts.array_dims[0] = list_node->n_children;
    for (int i = 1; i < 8; ++i) array_ts.array_dims[i] = -1;

    Node array_tmp{};
    array_tmp.kind = NK_COMPOUND_LIT;
    array_tmp.type = array_ts;
    array_tmp.left = const_cast<Node*>(list_node);
    array_tmp.line = list_node->line;

    ExprId array_id = lower_expr(ctx, &array_tmp);

    TypeSpec idx_ts{};
    idx_ts.base = TB_INT;
    ExprId zero_idx = append_expr(list_node, IntLiteral{0, false}, idx_ts);
    IndexExpr first_elem{};
    first_elem.base = array_id;
    first_elem.index = zero_idx;
    ExprId first_elem_id =
        append_expr(list_node, first_elem, elem_ts, ValueCategory::LValue);

    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = first_elem_id;
    data_ptr_id = append_expr(list_node, addr, data_ptr_ts);
  } else {
    data_ptr_id = append_expr(list_node, IntLiteral{0, false}, data_ptr_ts);
  }

  LocalDecl tmp{};
  tmp.id = next_local_id();
  tmp.name = "__init_list_arg_" + std::to_string(tmp.id.value);
  tmp.type = qtype_from(param_ts, ValueCategory::LValue);
  tmp.storage = StorageClass::Auto;
  tmp.span = make_span(list_node);
  const std::string tmp_name = tmp.name;
  TypeSpec int_ts{};
  int_ts.base = TB_INT;
  tmp.init = append_expr(list_node, IntLiteral{0, false}, int_ts);
  const LocalId tmp_lid = tmp.id;
  ctx->locals[tmp_name] = tmp.id;
  ctx->local_types[tmp.id.value] = param_ts;
  append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(list_node)});

  DeclRef tmp_ref{};
  tmp_ref.name = tmp_name;
  tmp_ref.local = tmp_lid;
  ExprId tmp_id = append_expr(list_node, tmp_ref, param_ts, ValueCategory::LValue);

  auto assign_field = [&](const char* field_name, const TypeSpec& field_ts, ExprId rhs_id) {
    MemberExpr lhs{};
    lhs.base = tmp_id;
    lhs.field = field_name;
    lhs.is_arrow = false;
    ExprId lhs_id = append_expr(list_node, lhs, field_ts, ValueCategory::LValue);
    AssignExpr assign{};
    assign.op = AssignOp::Set;
    assign.lhs = lhs_id;
    assign.rhs = rhs_id;
    ExprId assign_id = append_expr(list_node, assign, field_ts);
    append_stmt(*ctx, Stmt{StmtPayload{ExprStmt{assign_id}}, make_span(list_node)});
  };

  assign_field("_M_array", data_ptr_ts, data_ptr_id);
  ExprId len_id = append_expr(
      list_node, IntLiteral{list_node ? list_node->n_children : 0, false}, len_ts);
  assign_field("_M_len", len_ts, len_id);
  return tmp_id;
}

ExprId Lowerer::lower_stmt_expr_block(FunctionCtx& ctx,
                                      const Node* block,
                                      const TypeSpec& result_ts) {
  if (!block || block->kind != NK_BLOCK) {
    TypeSpec ts = result_ts;
    if (ts.base == TB_VOID) ts.base = TB_INT;
    return append_expr(nullptr, IntLiteral{0, false}, ts);
  }

  const bool new_scope = (block->ival != 1);
  const auto saved_locals = ctx.locals;
  const auto saved_static_globals = ctx.static_globals;

  ExprId result{};
  bool have_result = false;
  for (int i = 0; i < block->n_children; ++i) {
    const Node* child = block->children[i];
    const bool is_last = (i + 1 == block->n_children);
    if (is_last && child && child->kind == NK_EXPR_STMT && child->left &&
        result_ts.base != TB_VOID) {
      result = lower_expr(&ctx, child->left);
      have_result = true;
      continue;
    }
    lower_stmt_node(ctx, child);
  }

  if (new_scope) {
    ctx.locals = saved_locals;
    ctx.static_globals = saved_static_globals;
  }

  if (have_result) return result;
  TypeSpec ts = result_ts;
  if (ts.base == TB_VOID) ts.base = TB_INT;
  return append_expr(nullptr, IntLiteral{0, false}, ts);
}

ExprId Lowerer::try_lower_operator_call(FunctionCtx* ctx,
                                        const Node* result_node,
                                        const Node* obj_node,
                                        const char* op_method_name,
                                        const std::vector<const Node*>& arg_nodes,
                                        const std::vector<ExprId>& extra_args) {
  TypeSpec obj_ts = infer_generic_ctrl_type(ctx, obj_node);
  if (obj_ts.ptr_level != 0 || obj_ts.base != TB_STRUCT || !obj_ts.tag) {
    return ExprId::invalid();
  }

  std::optional<std::string> resolved =
      find_struct_method_mangled(obj_ts.tag, op_method_name, obj_ts.is_const);
  if (!resolved) return ExprId::invalid();

  std::string resolved_mangled = *resolved;
  std::string base_key = std::string(obj_ts.tag) + "::" + op_method_name;
  auto ovit = ref_overload_set_.find(base_key);
  if (ovit != ref_overload_set_.end() && !ovit->second.empty()) {
    const auto& overloads = ovit->second;
    const std::string* best_name = nullptr;
    int best_score = -1;
    for (const auto& ov : overloads) {
      bool viable = true;
      int score = 0;
      for (size_t i = 0; i < arg_nodes.size() && i < ov.param_is_rvalue_ref.size(); ++i) {
        bool arg_is_lvalue = is_ast_lvalue(arg_nodes[i]);
        if (ov.param_is_lvalue_ref[i] && !arg_is_lvalue) {
          viable = false;
          break;
        }
        if (ov.param_is_rvalue_ref[i] && arg_is_lvalue) {
          viable = false;
          break;
        }
        if (ov.param_is_rvalue_ref[i] && !arg_is_lvalue) score += 2;
        else if (ov.param_is_lvalue_ref[i] && arg_is_lvalue) score += 2;
        else score += 1;
      }
      if (viable && score > best_score) {
        best_name = &ov.mangled_name;
        best_score = score;
      }
    }
    if (best_name) resolved_mangled = *best_name;
  }

  CallExpr c{};
  DeclRef dr{};
  dr.name = resolved_mangled;
  auto fit = module_->fn_index.find(dr.name);
  TypeSpec fn_ts{};
  fn_ts.base = TB_VOID;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    fn_ts = module_->functions[fit->second.value].return_type.spec;
  }
  if (fn_ts.base == TB_VOID) {
    if (auto rit = find_struct_method_return_type(obj_ts.tag, op_method_name, obj_ts.is_const)) {
      fn_ts = *rit;
    }
  }
  TypeSpec callee_ts = fn_ts;
  callee_ts.ptr_level++;
  c.callee = append_expr(result_node, dr, callee_ts);

  ExprId obj_id = lower_expr(ctx, obj_node);
  if (ctx && module_->expr_pool[obj_id.value].type.category != ValueCategory::LValue) {
    LocalDecl tmp{};
    tmp.id = next_local_id();
    std::string tmp_name = "__op_call_tmp_" + std::to_string(tmp.id.value);
    tmp.name = tmp_name;
    tmp.type = qtype_from(obj_ts, ValueCategory::RValue);
    const TypeSpec init_ts = module_->expr_pool[obj_id.value].type.spec;
    if (init_ts.base == obj_ts.base && init_ts.ptr_level == obj_ts.ptr_level &&
        init_ts.tag == obj_ts.tag) {
      tmp.init = obj_id;
    }
    const LocalId tmp_lid = tmp.id;
    append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(obj_node)});

    DeclRef tmp_ref{};
    tmp_ref.name = tmp_name;
    tmp_ref.local = tmp_lid;
    obj_id = append_expr(obj_node, tmp_ref, obj_ts, ValueCategory::LValue);
  }
  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = obj_id;
  TypeSpec ptr_ts = obj_ts;
  ptr_ts.ptr_level++;
  c.args.push_back(append_expr(obj_node, addr, ptr_ts));

  const Function* callee_fn = nullptr;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    callee_fn = &module_->functions[fit->second.value];
  }
  const Node* method_ast = nullptr;
  if (!callee_fn) {
    for (const auto& pm : pending_methods_) {
      if (pm.mangled == resolved_mangled) {
        method_ast = pm.method_node;
        break;
      }
    }
  }

  auto lower_operator_ref_arg = [&](const Node* arg_node, const TypeSpec* param_ts) -> ExprId {
    if (!param_ts || (!param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref)) {
      return lower_expr(ctx, arg_node);
    }
    TypeSpec storage_ts = reference_storage_ts(*param_ts);
    if (param_ts->is_rvalue_ref) {
      if (arg_node && arg_node->kind == NK_CAST && arg_node->type.is_rvalue_ref &&
          arg_node->left) {
        UnaryExpr addr_e{};
        addr_e.op = UnaryOp::AddrOf;
        addr_e.operand = lower_expr(ctx, arg_node->left);
        return append_expr(arg_node, addr_e, storage_ts);
      }
      ExprId arg_val = lower_expr(ctx, arg_node);
      TypeSpec val_ts = reference_value_ts(*param_ts);
      resolve_typedef_to_struct(val_ts);
      LocalDecl tmp{};
      tmp.id = next_local_id();
      tmp.name = "__rref_arg_tmp";
      tmp.type = qtype_from(val_ts, ValueCategory::LValue);
      tmp.init = arg_val;
      const LocalId tmp_lid = tmp.id;
      ctx->locals[tmp.name] = tmp.id;
      ctx->local_types[tmp.id.value] = val_ts;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(arg_node)});
      DeclRef tmp_ref{};
      tmp_ref.name = "__rref_arg_tmp";
      tmp_ref.local = tmp_lid;
      ExprId var_id = append_expr(arg_node, tmp_ref, val_ts, ValueCategory::LValue);
      UnaryExpr addr_e{};
      addr_e.op = UnaryOp::AddrOf;
      addr_e.operand = var_id;
      return append_expr(arg_node, addr_e, storage_ts);
    }
    UnaryExpr addr_e{};
    addr_e.op = UnaryOp::AddrOf;
    addr_e.operand = lower_expr(ctx, arg_node);
    return append_expr(arg_node, addr_e, storage_ts);
  };

  for (size_t i = 0; i < arg_nodes.size(); ++i) {
    const TypeSpec* param_ts = (callee_fn && (i + 1) < callee_fn->params.size())
                                   ? &callee_fn->params[i + 1].type.spec
                                   : nullptr;
    TypeSpec ast_param_ts{};
    if (!param_ts && method_ast && static_cast<int>(i) < method_ast->n_params) {
      ast_param_ts = method_ast->params[i]->type;
      param_ts = &ast_param_ts;
    }
    if (param_ts && (param_ts->is_rvalue_ref || param_ts->is_lvalue_ref)) {
      c.args.push_back(lower_operator_ref_arg(arg_nodes[i], param_ts));
    } else {
      c.args.push_back(lower_expr(ctx, arg_nodes[i]));
    }
  }
  for (auto ea : extra_args) c.args.push_back(ea);

  return append_expr(result_node, c, fn_ts);
}

ExprId Lowerer::maybe_bool_convert(FunctionCtx* ctx, ExprId expr, const Node* n) {
  if (!expr.valid() || !n) return expr;
  TypeSpec ts = infer_generic_ctrl_type(ctx, n);
  if (ts.ptr_level != 0 || ts.base != TB_STRUCT || !ts.tag) return expr;
  std::string base_key = std::string(ts.tag) + "::operator_bool";
  std::string const_key = base_key + "_const";
  auto mit = struct_methods_.find(base_key);
  if (mit == struct_methods_.end()) mit = struct_methods_.find(const_key);
  if (mit == struct_methods_.end()) return expr;

  CallExpr c{};
  DeclRef dr{};
  dr.name = mit->second;
  auto fit = module_->fn_index.find(dr.name);
  TypeSpec fn_ts{};
  fn_ts.base = TB_BOOL;
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    fn_ts = module_->functions[fit->second.value].return_type.spec;
  }
  TypeSpec callee_ts = fn_ts;
  callee_ts.ptr_level++;
  c.callee = append_expr(n, dr, callee_ts);

  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = expr;
  TypeSpec ptr_ts = ts;
  ptr_ts.ptr_level++;
  c.args.push_back(append_expr(n, addr, ptr_ts));

  return append_expr(n, c, fn_ts);
}

TypeSpec Lowerer::builtin_query_result_type() const {
  TypeSpec ts{};
  ts.base = TB_ULONG;
  return ts;
}

TypeSpec Lowerer::resolve_builtin_query_type(FunctionCtx* ctx, TypeSpec target) const {
  if (!ctx || ctx->tpl_bindings.empty() ||
      target.base != TB_TYPEDEF || !target.tag) {
    return target;
  }
  auto it = ctx->tpl_bindings.find(target.tag);
  if (it == ctx->tpl_bindings.end()) return target;
  const TypeSpec& concrete = it->second;
  target.base = concrete.base;
  target.tag = concrete.tag;
  if (concrete.ptr_level > 0) target.ptr_level += concrete.ptr_level;
  return target;
}

ExprId Lowerer::lower_builtin_sizeof_type(FunctionCtx* ctx, const Node* n) {
  const LayoutQueries queries(*module_);
  const TypeSpec sizeof_target = resolve_builtin_query_type(ctx, n->type);
  const TypeSpec result_ts = builtin_query_result_type();
  if (ctx && sizeof_target.array_rank > 0 && n->type.array_size_expr &&
      (sizeof_target.array_size <= 0 || sizeof_target.array_dims[0] <= 0)) {
    TypeSpec elem_ts = sizeof_target;
    elem_ts.array_rank--;
    if (elem_ts.array_rank > 0) {
      for (int i = 0; i < elem_ts.array_rank; ++i) {
        elem_ts.array_dims[i] = elem_ts.array_dims[i + 1];
      }
    }
    elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;

    const ExprId count_id = lower_expr(ctx, n->type.array_size_expr);
    const ExprId elem_size_id = append_expr(
        n, IntLiteral{static_cast<long long>(queries.type_size_bytes(elem_ts)), false},
        result_ts);
    BinaryExpr mul{};
    mul.op = BinaryOp::Mul;
    mul.lhs = count_id;
    mul.rhs = elem_size_id;
    return append_expr(n, mul, result_ts);
  }

  const int size = queries.type_size_bytes(sizeof_target);
  return append_expr(n, IntLiteral{static_cast<long long>(size), false}, result_ts);
}

ExprId Lowerer::lower_builtin_alignof_type(FunctionCtx* ctx, const Node* n) {
  const LayoutQueries queries(*module_);
  const TypeSpec alignof_target = resolve_builtin_query_type(ctx, n->type);
  const int align = queries.type_align_bytes(alignof_target);
  return append_expr(
      n, IntLiteral{static_cast<long long>(align), false}, builtin_query_result_type());
}

int Lowerer::builtin_alignof_expr_bytes(FunctionCtx* ctx, const Node* expr) {
  const LayoutQueries queries(*module_);
  const TypeSpec expr_ts = infer_generic_ctrl_type(ctx, expr);
  int align = 0;
  if (expr && expr->kind == NK_VAR && expr->name) {
    const std::string fn_name(expr->name);
    auto fit = module_->fn_index.find(fn_name);
    if (fit != module_->fn_index.end() &&
        fit->second.value < module_->functions.size()) {
      int fn_align = module_->functions[fit->second.value].attrs.align_bytes;
      if (fn_align > 0) align = fn_align;
    }
  }
  if (align != 0) return align;
  if (expr_ts.align_bytes > 0) return expr_ts.align_bytes;
  return queries.type_align_bytes(expr_ts);
}

ExprId Lowerer::lower_builtin_alignof_expr(FunctionCtx* ctx, const Node* n) {
  const int align = builtin_alignof_expr_bytes(ctx, n->left);
  return append_expr(
      n, IntLiteral{static_cast<long long>(align), false}, builtin_query_result_type());
}

ExprId Lowerer::lower_expr(FunctionCtx* ctx, const Node* n) {
  if (!n) {
    TypeSpec ts{};
    ts.base = TB_INT;
    return append_expr(nullptr, IntLiteral{0, false}, ts);
  }

  switch (n->kind) {
    case NK_INT_LIT: {
      if (n->name && n->name[0]) {
        LabelAddrExpr la{};
        la.label_name = n->name;
        la.fn_name = ctx ? ctx->fn->name : "";
        TypeSpec void_ptr{};
        void_ptr.base = TB_VOID;
        void_ptr.ptr_level = 1;
        return append_expr(n, std::move(la), void_ptr);
      }
      return append_expr(n, IntLiteral{n->ival, false}, infer_int_literal_type(n));
    }
    case NK_FLOAT_LIT: {
      TypeSpec ts = n->type;
      if (!has_concrete_type(ts)) ts = classify_float_literal_type(const_cast<Node*>(n));
      return append_expr(n, FloatLiteral{n->fval}, ts);
    }
    case NK_STR_LIT: {
      StringLiteral s{};
      s.raw = n->sval ? n->sval : "";
      s.is_wide = s.raw.size() >= 2 && s.raw[0] == 'L' && s.raw[1] == '"';
      TypeSpec ts = n->type;
      return append_expr(n, std::move(s), ts, ValueCategory::LValue);
    }
    case NK_CHAR_LIT: {
      TypeSpec ts = n->type;
      ts.base = TB_INT;
      return append_expr(n, CharLiteral{n->ival}, ts);
    }
    case NK_PACK_EXPANSION:
      return lower_expr(ctx, n->left);
    case NK_VAR: {
      if (n->name && n->name[0]) {
        if (n->has_template_args && find_template_struct_primary(n->name)) {
          std::string arg_refs;
          for (int i = 0; i < n->n_template_args; ++i) {
            if (!arg_refs.empty()) arg_refs += ",";
            if (n->template_arg_is_value && n->template_arg_is_value[i]) {
              const char* fwd_name = n->template_arg_nttp_names ?
                  n->template_arg_nttp_names[i] : nullptr;
              if (fwd_name && fwd_name[0]) arg_refs += fwd_name;
              else arg_refs += std::to_string(n->template_arg_values[i]);
            } else if (n->template_arg_types) {
              const TypeSpec& arg_ts = n->template_arg_types[i];
              arg_refs += encode_template_type_arg_ref_hir(arg_ts);
            }
          }
          TypeSpec tmp_ts{};
          tmp_ts.base = TB_STRUCT;
          tmp_ts.array_size = -1;
          tmp_ts.inner_rank = -1;
          tmp_ts.tpl_struct_origin = n->name;
          tmp_ts.tpl_struct_arg_refs = ::strdup(arg_refs.c_str());
          const Node* primary_tpl = find_template_struct_primary(n->name);
          TypeBindings tpl_empty;
          NttpBindings nttp_empty;
          seed_and_resolve_pending_template_type_if_needed(
              tmp_ts, ctx ? ctx->tpl_bindings : tpl_empty,
              ctx ? ctx->nttp_bindings : nttp_empty, n,
              PendingTemplateTypeKind::OwnerStruct, "nameref-tpl-ctor",
              primary_tpl);
          if (tmp_ts.tag && module_->struct_defs.count(tmp_ts.tag)) {
            const LocalId tmp_lid = next_local_id();
            const std::string tmp_name = "__tmp_struct_" + std::to_string(tmp_lid.value);
            LocalDecl tmp{};
            tmp.id = tmp_lid;
            tmp.name = tmp_name;
            tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
            tmp.storage = StorageClass::Auto;
            tmp.span = make_span(n);
            if (ctx) {
              ctx->locals[tmp.name] = tmp.id;
              ctx->local_types[tmp.id.value] = tmp_ts;
              append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
              DeclRef tmp_ref{};
              tmp_ref.name = tmp_name;
              tmp_ref.local = tmp_lid;
              return append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
            }
          }
        }
        std::string qname = n->name;
        size_t scope_pos = qname.rfind("::");
        if (scope_pos != std::string::npos) {
          std::string struct_tag = qname.substr(0, scope_pos);
          std::string member = qname.substr(scope_pos + 2);
          if (!find_struct_static_member_decl(struct_tag, member) &&
              n->has_template_args && find_template_struct_primary(struct_tag)) {
            std::string arg_refs;
            for (int i = 0; i < n->n_template_args; ++i) {
              if (!arg_refs.empty()) arg_refs += ",";
              if (n->template_arg_is_value && n->template_arg_is_value[i]) {
                const char* fwd_name = n->template_arg_nttp_names ?
                    n->template_arg_nttp_names[i] : nullptr;
                if (fwd_name && fwd_name[0]) arg_refs += fwd_name;
                else arg_refs += std::to_string(n->template_arg_values[i]);
              } else if (n->template_arg_types) {
                const TypeSpec& arg_ts = n->template_arg_types[i];
                arg_refs += encode_template_type_arg_ref_hir(arg_ts);
              }
            }
            TypeSpec pending_ts{};
            pending_ts.base = TB_STRUCT;
            pending_ts.array_size = -1;
            pending_ts.inner_rank = -1;
            pending_ts.tpl_struct_origin = ::strdup(struct_tag.c_str());
            pending_ts.tpl_struct_arg_refs = ::strdup(arg_refs.c_str());
            const Node* primary_tpl = find_template_struct_primary(struct_tag);
            TypeBindings tpl_empty;
            NttpBindings nttp_empty;
            seed_and_resolve_pending_template_type_if_needed(
                pending_ts, ctx ? ctx->tpl_bindings : tpl_empty,
                ctx ? ctx->nttp_bindings : nttp_empty, n,
                PendingTemplateTypeKind::OwnerStruct, "nameref-scope-tpl",
                primary_tpl);
            if (pending_ts.tag && pending_ts.tag[0]) struct_tag = pending_ts.tag;
          }
          if (auto v = find_struct_static_member_const_value(struct_tag, member)) {
            TypeSpec ts{};
            if (const Node* decl = find_struct_static_member_decl(struct_tag, member))
              ts = decl->type;
            if (ts.base == TB_VOID) ts.base = TB_INT;
            return append_expr(n, IntLiteral{*v, false}, ts);
          }
          if (const Node* decl = find_struct_static_member_decl(struct_tag, member)) {
            if (decl->init) {
              long long v = static_eval_int(decl->init, enum_consts_);
              TypeSpec ts = decl->type;
              if (ts.base == TB_VOID) ts.base = TB_INT;
              return append_expr(n, IntLiteral{v, false}, ts);
            }
          }
        }
        auto it = enum_consts_.find(n->name);
        if (it != enum_consts_.end()) {
          TypeSpec ts{};
          ts.base = TB_INT;
          return append_expr(n, IntLiteral{it->second, false}, ts);
        }
        if (ctx) {
          auto nttp_it = ctx->nttp_bindings.find(n->name);
          if (nttp_it != ctx->nttp_bindings.end()) {
            TypeSpec ts{};
            ts.base = TB_INT;
            ts.array_size = -1;
            ts.inner_rank = -1;
            return append_expr(n, IntLiteral{nttp_it->second, false}, ts);
          }
        }
      }
      DeclRef r{};
      r.name = n->name ? n->name : "<anon_var>";
      r.ns_qual = make_ns_qual(n);
      bool has_local_binding = false;
      if (ctx) {
        auto lit = ctx->locals.find(r.name);
        if (lit != ctx->locals.end()) {
          r.local = lit->second;
          has_local_binding = true;
        }
        auto sit = ctx->static_globals.find(r.name);
        if (sit != ctx->static_globals.end()) {
          r.global = sit->second;
          if (const GlobalVar* gv = module_->find_global(*r.global)) r.name = gv->name;
          has_local_binding = true;
        }
        if (!has_local_binding) {
          auto pit = ctx->params.find(r.name);
          if (pit != ctx->params.end()) r.param_index = pit->second;
        }
      }
      if (!has_local_binding) {
        auto git = module_->global_index.find(r.name);
        if (git != module_->global_index.end()) r.global = git->second;
      }
      if (ctx && !ctx->method_struct_tag.empty() && !has_local_binding &&
          !r.param_index && !r.global) {
        if (auto v = find_struct_static_member_const_value(ctx->method_struct_tag, r.name)) {
          TypeSpec ts{};
          if (const Node* decl =
                  find_struct_static_member_decl(ctx->method_struct_tag, r.name)) {
            ts = decl->type;
          }
          if (ts.base == TB_VOID) ts.base = TB_INT;
          return append_expr(n, IntLiteral{*v, false}, ts);
        }
        auto sit = module_->struct_defs.find(ctx->method_struct_tag);
        if (sit != module_->struct_defs.end()) {
          for (const auto& fld : sit->second.fields) {
            if (fld.name == r.name) {
              DeclRef this_ref{};
              this_ref.name = "this";
              auto pit = ctx->params.find("this");
              if (pit != ctx->params.end()) this_ref.param_index = pit->second;
              TypeSpec this_ts{};
              this_ts.base = TB_STRUCT;
              this_ts.tag = sit->second.tag.c_str();
              this_ts.ptr_level = 1;
              ExprId this_id = append_expr(n, this_ref, this_ts, ValueCategory::LValue);
              MemberExpr me{};
              me.base = this_id;
              me.field = r.name;
              me.is_arrow = true;
              return append_expr(n, me, n->type, ValueCategory::LValue);
            }
          }
        }
      }
      TypeSpec var_ts = n->type;
      if (var_ts.base == TB_VOID && var_ts.ptr_level == 0 && var_ts.array_rank == 0 &&
          !r.local && !r.param_index && !r.global) {
        auto fit = module_->fn_index.find(r.name);
        if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
          var_ts = module_->functions[fit->second.value].return_type.spec;
          var_ts.ptr_level++;
          var_ts.is_fn_ptr = true;
        }
      }
      ExprId ref_id = append_expr(n, r, var_ts, ValueCategory::LValue);
      if (ctx) {
        if (auto storage_ts = storage_type_for_declref(ctx, r);
            storage_ts && is_any_ref_ts(*storage_ts)) {
          UnaryExpr u{};
          u.op = UnaryOp::Deref;
          u.operand = ref_id;
          return append_expr(n, u, reference_value_ts(*storage_ts), ValueCategory::LValue);
        }
      }
      return ref_id;
    }
    case NK_UNARY: {
      if (n->op) {
        const char* op_name = nullptr;
        if (std::string(n->op) == "++pre") op_name = "operator_preinc";
        else if (std::string(n->op) == "--pre") op_name = "operator_predec";
        else if (std::string(n->op) == "+") op_name = "operator_plus";
        else if (std::string(n->op) == "-") op_name = "operator_minus";
        else if (std::string(n->op) == "!") op_name = "operator_not";
        else if (std::string(n->op) == "~") op_name = "operator_bitnot";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {});
          if (op.valid()) return op;
        }
      }
      UnaryExpr u{};
      u.op = map_unary_op(n->op);
      u.operand = lower_expr(ctx, n->left);
      if (u.op == UnaryOp::Not) u.operand = maybe_bool_convert(ctx, u.operand, n->left);
      return append_expr(n, u, n->type);
    }
    case NK_POSTFIX: {
      if (n->op) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "++") op_name = "operator_postinc";
        else if (op_str == "--") op_name = "operator_postdec";
        if (op_name) {
          TypeSpec int_ts{};
          int_ts.base = TB_INT;
          ExprId dummy = append_expr(n, IntLiteral{0, false}, int_ts);
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {}, {dummy});
          if (op.valid()) return op;
        }
      }
      UnaryExpr u{};
      const std::string op = n->op ? n->op : "";
      u.op = (op == "--") ? UnaryOp::PostDec : UnaryOp::PostInc;
      u.operand = lower_expr(ctx, n->left);
      return append_expr(n, u, n->type);
    }
    case NK_ADDR: {
      {
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_addr", {});
        if (op.valid()) return op;
      }
      if (n->left && n->left->kind == NK_VAR && n->left->name &&
          ct_state_->has_consteval_def(n->left->name)) {
        std::string diag = "error: cannot take address of consteval function '";
        diag += n->left->name;
        diag += "'";
        throw std::runtime_error(diag);
      }
      UnaryExpr u{};
      u.op = UnaryOp::AddrOf;
      u.operand = lower_expr(ctx, n->left);
      return append_expr(n, u, n->type);
    }
    case NK_DEREF: {
      {
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_deref", {});
        if (op.valid()) return op;
      }
      UnaryExpr u{};
      u.op = UnaryOp::Deref;
      u.operand = lower_expr(ctx, n->left);
      return append_expr(n, u, n->type, ValueCategory::LValue);
    }
    case NK_COMMA_EXPR: {
      BinaryExpr b{};
      b.op = BinaryOp::Comma;
      b.lhs = lower_expr(ctx, n->left);
      b.rhs = lower_expr(ctx, n->right);
      return append_expr(n, b, n->type);
    }
    case NK_BINOP: {
      if (n->op && n->right) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "==") op_name = "operator_eq";
        else if (op_str == "!=") op_name = "operator_neq";
        else if (op_str == "+") op_name = "operator_plus";
        else if (op_str == "-") op_name = "operator_minus";
        else if (op_str == "*") op_name = "operator_mul";
        else if (op_str == "/") op_name = "operator_div";
        else if (op_str == "%") op_name = "operator_mod";
        else if (op_str == "&") op_name = "operator_bitand";
        else if (op_str == "|") op_name = "operator_bitor";
        else if (op_str == "^") op_name = "operator_bitxor";
        else if (op_str == "<<") op_name = "operator_shl";
        else if (op_str == ">>") op_name = "operator_shr";
        else if (op_str == "<") op_name = "operator_lt";
        else if (op_str == ">") op_name = "operator_gt";
        else if (op_str == "<=") op_name = "operator_le";
        else if (op_str == ">=") op_name = "operator_ge";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      BinaryExpr b{};
      b.op = map_binary_op(n->op);
      b.lhs = lower_expr(ctx, n->left);
      b.rhs = lower_expr(ctx, n->right);
      if (b.op == BinaryOp::LAnd || b.op == BinaryOp::LOr) {
        b.lhs = maybe_bool_convert(ctx, b.lhs, n->left);
        b.rhs = maybe_bool_convert(ctx, b.rhs, n->right);
      }
      return append_expr(n, b, n->type);
    }
    case NK_ASSIGN: {
      if (n->kind == NK_ASSIGN && ctx && n->op) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "=") op_name = "operator_assign";
        else if (op_str == "+=") op_name = "operator_plus_assign";
        else if (op_str == "-=") op_name = "operator_minus_assign";
        else if (op_str == "*=") op_name = "operator_mul_assign";
        else if (op_str == "/=") op_name = "operator_div_assign";
        else if (op_str == "%=") op_name = "operator_mod_assign";
        else if (op_str == "&=") op_name = "operator_and_assign";
        else if (op_str == "|=") op_name = "operator_or_assign";
        else if (op_str == "^=") op_name = "operator_xor_assign";
        else if (op_str == "<<=") op_name = "operator_shl_assign";
        else if (op_str == ">>=") op_name = "operator_shr_assign";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      AssignExpr a{};
      a.op = map_assign_op(n->op, n->kind);
      a.lhs = lower_expr(ctx, n->left);
      a.rhs = lower_expr(ctx, n->right);
      return append_expr(n, a, n->type);
    }
    case NK_COMPOUND_ASSIGN: {
      if (n->op && ctx) {
        const char* op_name = nullptr;
        std::string op_str(n->op);
        if (op_str == "+=") op_name = "operator_plus_assign";
        else if (op_str == "-=") op_name = "operator_minus_assign";
        else if (op_str == "*=") op_name = "operator_mul_assign";
        else if (op_str == "/=") op_name = "operator_div_assign";
        else if (op_str == "%=") op_name = "operator_mod_assign";
        else if (op_str == "&=") op_name = "operator_and_assign";
        else if (op_str == "|=") op_name = "operator_or_assign";
        else if (op_str == "^=") op_name = "operator_xor_assign";
        else if (op_str == "<<=") op_name = "operator_shl_assign";
        else if (op_str == ">>=") op_name = "operator_shr_assign";
        if (op_name) {
          ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
          if (op.valid()) return op;
        }
      }
      AssignExpr a{};
      a.op = map_assign_op(n->op, n->kind);
      a.lhs = lower_expr(ctx, n->left);
      a.rhs = lower_expr(ctx, n->right);
      return append_expr(n, a, n->type);
    }
    case NK_CAST: {
      CastExpr c{};
      TypeSpec cast_ts = n->type;
      if (ctx && !ctx->tpl_bindings.empty() && cast_ts.base == TB_TYPEDEF && cast_ts.tag) {
        auto it = ctx->tpl_bindings.find(cast_ts.tag);
        if (it != ctx->tpl_bindings.end()) {
          const TypeSpec& concrete = it->second;
          cast_ts.base = concrete.base;
          cast_ts.tag = concrete.tag;
        }
      }
      if (ctx && !ctx->tpl_bindings.empty() && cast_ts.tpl_struct_origin) {
        seed_and_resolve_pending_template_type_if_needed(
            cast_ts, ctx->tpl_bindings, ctx->nttp_bindings, n,
            PendingTemplateTypeKind::CastTarget, "cast-target");
      }
      c.to_type = qtype_from(cast_ts);
      c.expr = lower_expr(ctx, n->left);
      if (cast_ts.is_fn_ptr) c.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
      return append_expr(n, c, cast_ts);
    }
    case NK_CALL:
    case NK_BUILTIN_CALL:
      return lower_call_expr(ctx, n);
    case NK_VA_ARG: {
      VaArgExpr v{};
      v.ap = lower_expr(ctx, n->left);
      return append_expr(n, v, n->type);
    }
    case NK_INDEX: {
      if (n->right) {
        ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_subscript", {n->right});
        if (op.valid()) return op;
      }
      IndexExpr idx{};
      idx.base = lower_expr(ctx, n->left);
      idx.index = lower_expr(ctx, n->right);
      return append_expr(n, idx, n->type, ValueCategory::LValue);
    }
    case NK_MEMBER: {
      if (n->is_arrow && n->left) {
        ExprId arrow_ptr = try_lower_operator_call(ctx, n, n->left, "operator_arrow", {});
        if (arrow_ptr.valid()) {
          for (int depth = 0; depth < 8; ++depth) {
            const Expr* res_expr = module_->find_expr(arrow_ptr);
            if (!res_expr) break;
            TypeSpec rts = res_expr->type.spec;
            if (rts.ptr_level > 0) break;
            if (rts.base != TB_STRUCT || !rts.tag) break;
            std::string base_key = std::string(rts.tag) + "::operator_arrow";
            std::string const_key = base_key + "_const";
            auto mit = rts.is_const ? struct_methods_.find(const_key) : struct_methods_.find(base_key);
            if (mit == struct_methods_.end()) {
              mit = rts.is_const ? struct_methods_.find(base_key) : struct_methods_.find(const_key);
            }
            if (mit == struct_methods_.end()) break;
            auto fit = module_->fn_index.find(mit->second);
            TypeSpec fn_ts{};
            fn_ts.base = TB_VOID;
            if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
              fn_ts = module_->functions[fit->second.value].return_type.spec;
            }
            if (fn_ts.base == TB_VOID) {
              auto rit2 = struct_method_ret_types_.find(std::string(rts.tag) + "::operator_arrow");
              if (rit2 == struct_method_ret_types_.end()) {
                rit2 = struct_method_ret_types_.find(std::string(rts.tag) + "::operator_arrow_const");
              }
              if (rit2 != struct_method_ret_types_.end()) fn_ts = rit2->second;
            }
            LocalDecl tmp{};
            tmp.id = next_local_id();
            tmp.name = "__arrow_tmp";
            tmp.type = qtype_from(rts, ValueCategory::LValue);
            tmp.init = arrow_ptr;
            const LocalId tmp_lid = tmp.id;
            ctx->locals[tmp.name] = tmp.id;
            ctx->local_types[tmp.id.value] = rts;
            append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
            DeclRef tmp_ref{};
            tmp_ref.name = "__arrow_tmp";
            tmp_ref.local = tmp_lid;
            ExprId tmp_id = append_expr(n, tmp_ref, rts, ValueCategory::LValue);
            CallExpr cc{};
            DeclRef dr{};
            dr.name = mit->second;
            TypeSpec callee_ts = fn_ts;
            callee_ts.ptr_level++;
            cc.callee = append_expr(n, dr, callee_ts);
            UnaryExpr addr{};
            addr.op = UnaryOp::AddrOf;
            addr.operand = tmp_id;
            TypeSpec ptr_ts = rts;
            ptr_ts.ptr_level++;
            cc.args.push_back(append_expr(n, addr, ptr_ts));
            arrow_ptr = append_expr(n, cc, fn_ts);
          }
          MemberExpr m{};
          m.base = arrow_ptr;
          m.field = n->name ? n->name : "<anon_field>";
          m.is_arrow = true;
          return append_expr(n, m, n->type, ValueCategory::LValue);
        }
      }
      MemberExpr m{};
      m.base = lower_expr(ctx, n->left);
      m.field = n->name ? n->name : "<anon_field>";
      m.is_arrow = n->is_arrow;
      return append_expr(n, m, n->type, ValueCategory::LValue);
    }
    case NK_TERNARY: {
      if (const Node* cond = (n->cond ? n->cond : n->left)) {
        ConstEvalEnv env{&enum_consts_, nullptr, ctx ? &ctx->local_const_bindings : nullptr};
        if (auto r = evaluate_constant_expr(cond, env); r.ok()) {
          return lower_expr(ctx, (r.as_int() != 0) ? n->then_ : n->else_);
        }
      }
      if (ctx && (contains_stmt_expr(n->then_) || contains_stmt_expr(n->else_))) {
        TypeSpec result_ts = n->type;
        if (result_ts.base == TB_VOID) result_ts.base = TB_INT;
        LocalDecl tmp{};
        tmp.id = next_local_id();
        tmp.name = "__tern_tmp";
        tmp.type = qtype_from(result_ts, ValueCategory::LValue);
        TypeSpec zero_ts{};
        zero_ts.base = TB_INT;
        tmp.init = append_expr(n, IntLiteral{0, false}, zero_ts);
        const LocalId tmp_lid = tmp.id;
        ctx->locals[tmp.name] = tmp.id;
        ctx->local_types[tmp.id.value] = result_ts;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
        const Node* cond_n = n->cond ? n->cond : n->left;
        ExprId cond_expr = maybe_bool_convert(ctx, lower_expr(ctx, cond_n), cond_n);
        const BlockId then_b = create_block(*ctx);
        const BlockId else_b = create_block(*ctx);
        const BlockId after_b = create_block(*ctx);
        IfStmt ifs{};
        ifs.cond = cond_expr;
        ifs.then_block = then_b;
        ifs.else_block = else_b;
        ifs.after_block = after_b;
        append_stmt(*ctx, Stmt{StmtPayload{ifs}, make_span(n)});
        ensure_block(*ctx, ctx->current_block).has_explicit_terminator = true;
        auto emit_branch = [&](BlockId blk, const Node* branch) {
          ctx->current_block = blk;
          ExprId val = lower_expr(ctx, branch);
          DeclRef lhs_ref{};
          lhs_ref.name = "__tern_tmp";
          lhs_ref.local = tmp_lid;
          ExprId lhs_id = append_expr(n, lhs_ref, result_ts, ValueCategory::LValue);
          AssignExpr assign{};
          assign.op = AssignOp::Set;
          assign.lhs = lhs_id;
          assign.rhs = val;
          ExprId assign_id = append_expr(n, assign, result_ts);
          append_stmt(*ctx, Stmt{StmtPayload{ExprStmt{assign_id}}, make_span(n)});
          if (!ensure_block(*ctx, ctx->current_block).has_explicit_terminator) {
            GotoStmt j{};
            j.target.resolved_block = after_b;
            append_stmt(*ctx, Stmt{StmtPayload{j}, make_span(n)});
            ensure_block(*ctx, ctx->current_block).has_explicit_terminator = true;
          }
        };
        emit_branch(then_b, n->then_);
        emit_branch(else_b, n->else_);
        ctx->current_block = after_b;
        DeclRef ref{};
        ref.name = "__tern_tmp";
        ref.local = tmp_lid;
        return append_expr(n, ref, result_ts, ValueCategory::LValue);
      }
      TernaryExpr t{};
      {
        const Node* cond_n = n->cond ? n->cond : n->left;
        t.cond = maybe_bool_convert(ctx, lower_expr(ctx, cond_n), cond_n);
      }
      t.then_expr = lower_expr(ctx, n->then_);
      t.else_expr = lower_expr(ctx, n->else_);
      return append_expr(n, t, n->type);
    }
    case NK_GENERIC: {
      TypeSpec ctrl_ts = infer_generic_ctrl_type(ctx, n->left);
      const Node* selected = nullptr;
      const Node* default_expr = nullptr;
      for (int i = 0; i < n->n_children; ++i) {
        const Node* assoc = n->children[i];
        if (!assoc) continue;
        const Node* expr = assoc->left;
        if (!expr) continue;
        if (assoc->ival == 1) {
          if (!default_expr) default_expr = expr;
          continue;
        }
        if (generic_type_compatible(ctrl_ts, assoc->type)) {
          selected = expr;
          break;
        }
      }
      if (!selected) selected = default_expr;
      if (selected) return lower_expr(ctx, selected);
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID) ts.base = TB_INT;
      return append_expr(n, IntLiteral{0, false}, ts);
    }
    case NK_STMT_EXPR: {
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID) ts.base = TB_INT;
      if (ctx && n->body) return lower_stmt_expr_block(*ctx, n->body, ts);
      return append_expr(n, IntLiteral{0, false}, ts);
    }
    case NK_REAL_PART:
    case NK_IMAG_PART: {
      UnaryExpr u{};
      u.op = (n->kind == NK_REAL_PART) ? UnaryOp::RealPart : UnaryOp::ImagPart;
      u.operand = lower_expr(ctx, n->left);
      return append_expr(n, u, n->type,
                         (n->left && n->left->type.ptr_level == 0 &&
                          n->left->type.array_rank == 0)
                             ? ValueCategory::LValue
                             : ValueCategory::RValue);
    }
    case NK_SIZEOF_EXPR: {
      SizeofExpr s{};
      s.expr = lower_expr(ctx, n->left);
      TypeSpec ts{};
      ts.base = TB_ULONG;
      return append_expr(n, s, ts);
    }
    case NK_SIZEOF_PACK: {
      long long pack_size = 0;
      std::string pack_name = n->sval ? n->sval : "";
      if (pack_name.empty() && n->left && n->left->kind == NK_VAR && n->left->name) {
        pack_name = n->left->name;
      }
      if (ctx && !pack_name.empty()) {
        pack_size = count_pack_bindings_for_name(ctx->tpl_bindings, ctx->nttp_bindings, pack_name);
      }
      IntLiteral lit{};
      lit.value = pack_size;
      TypeSpec ts{};
      ts.base = TB_ULONG;
      return append_expr(n, lit, ts);
    }
    case NK_SIZEOF_TYPE:
      return lower_builtin_sizeof_type(ctx, n);
    case NK_ALIGNOF_TYPE:
      return lower_builtin_alignof_type(ctx, n);
    case NK_ALIGNOF_EXPR:
      return lower_builtin_alignof_expr(ctx, n);
    case NK_COMPOUND_LIT: {
      if (!ctx) {
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID) ts.base = TB_INT;
        return append_expr(n, IntLiteral{0, false}, ts);
      }
      const TypeSpec clit_ts = n->type;
      const Node* init_list = (n->left && n->left->kind == NK_INIT_LIST) ? n->left : nullptr;
      TypeSpec actual_ts = clit_ts;
      if (actual_ts.array_rank > 0 && actual_ts.array_size < 0 && init_list) {
        long long count = init_list->n_children;
        for (int ci = 0; ci < init_list->n_children; ++ci) {
          const Node* item = init_list->children[ci];
          if (item && item->kind == NK_INIT_ITEM && item->is_designated &&
              item->is_index_desig && item->desig_val + 1 > count) {
            count = item->desig_val + 1;
          }
        }
        actual_ts.array_size = count;
        actual_ts.array_dims[0] = count;
      }
      LocalDecl d{};
      d.id = next_local_id();
      d.name = "<clit>";
      d.type = qtype_from(actual_ts, ValueCategory::LValue);
      d.storage = StorageClass::Auto;
      d.span = make_span(n);
      const LocalId lid = d.id;
      const TypeSpec decl_ts = actual_ts;
      const bool is_struct_or_union =
          (decl_ts.base == TB_STRUCT || decl_ts.base == TB_UNION) && decl_ts.ptr_level == 0 &&
          decl_ts.array_rank == 0;
      const bool is_array = decl_ts.array_rank > 0;
      const bool is_vector = is_vector_ty(decl_ts);
      if ((is_struct_or_union || is_array || is_vector) && init_list) {
        TypeSpec int_ts{};
        int_ts.base = TB_INT;
        d.init = append_expr(n, IntLiteral{0, false}, int_ts);
      } else if (init_list && init_list->n_children > 0) {
        d.init = lower_expr(ctx, unwrap_init_scalar_value(init_list));
      } else if (n->left && !init_list) {
        d.init = lower_expr(ctx, n->left);
      }
      ctx->local_types[lid.value] = decl_ts;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});
      DeclRef dr{};
      dr.name = "<clit>";
      dr.local = lid;
      ExprId base_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
      if (init_list && (is_struct_or_union || is_array || is_vector)) {
        auto is_agg = [](const TypeSpec& ts) {
          return ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0 &&
                  ts.array_rank == 0) ||
                 is_vector_ty(ts);
        };
        auto append_assign = [&](ExprId lhs_id, const TypeSpec& lhs_ts, const Node* rhs_node) {
          if (!rhs_node) return;
          if (lhs_ts.array_rank == 1 && rhs_node->kind == NK_STR_LIT && rhs_node->sval) {
            TypeSpec elem_ts = lhs_ts;
            elem_ts.array_rank = 0;
            elem_ts.array_size = -1;
            if (elem_ts.ptr_level == 0 &&
                (elem_ts.base == TB_CHAR || elem_ts.base == TB_SCHAR || elem_ts.base == TB_UCHAR)) {
              const bool is_wide = rhs_node->sval[0] == 'L';
              const auto vals = decode_string_literal_values(rhs_node->sval, is_wide);
              const long long max_count = lhs_ts.array_size > 0 ? lhs_ts.array_size
                                                                : static_cast<long long>(vals.size());
              const long long emit_n =
                  std::min<long long>(max_count, static_cast<long long>(vals.size()));
              for (long long idx = 0; idx < emit_n; ++idx) {
                TypeSpec idx_ts{};
                idx_ts.base = TB_INT;
                ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
                IndexExpr ie{};
                ie.base = lhs_id;
                ie.index = idx_id;
                ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
                ExprId val_id =
                    append_expr(n, IntLiteral{vals[static_cast<size_t>(idx)], false}, idx_ts);
                AssignExpr ae{};
                ae.lhs = ie_id;
                ae.rhs = val_id;
                ExprId ae_id = append_expr(n, ae, elem_ts);
                ExprStmt es{};
                es.expr = ae_id;
                append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
              }
              return;
            }
          }
          const Node* val_node = unwrap_init_scalar_value(rhs_node);
          ExprId val_id = lower_expr(ctx, val_node);
          AssignExpr ae{};
          ae.lhs = lhs_id;
          ae.rhs = val_id;
          ExprId ae_id = append_expr(n, ae, lhs_ts);
          ExprStmt es{};
          es.expr = ae_id;
          append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
        };
        auto can_direct_assign_agg = [&](const TypeSpec& lhs_ts, const Node* rhs_node) -> bool {
          if (!rhs_node) return false;
          if (!is_agg(lhs_ts)) return false;
          const TypeSpec rhs_ts = infer_generic_ctrl_type(ctx, rhs_node);
          if (rhs_ts.ptr_level != 0 || rhs_ts.array_rank != 0) return false;
          if (rhs_ts.base != lhs_ts.base) return false;
          if (rhs_ts.base == TB_STRUCT || rhs_ts.base == TB_UNION || rhs_ts.base == TB_ENUM) {
            const char* lt = lhs_ts.tag ? lhs_ts.tag : "";
            const char* rt = rhs_ts.tag ? rhs_ts.tag : "";
            return std::strcmp(lt, rt) == 0;
          }
          return true;
        };
        auto is_direct_char_array_init = [&](const TypeSpec& lhs_ts, const Node* rhs_node) -> bool {
          if (!rhs_node) return false;
          if (lhs_ts.array_rank != 1 || lhs_ts.ptr_level != 0) return false;
          if (!(lhs_ts.base == TB_CHAR || lhs_ts.base == TB_SCHAR || lhs_ts.base == TB_UCHAR)) {
            return false;
          }
          return rhs_node->kind == NK_STR_LIT;
        };
        std::function<void(const TypeSpec&, ExprId, const Node*, int&)> consume_from_list;
        consume_from_list = [&](const TypeSpec& cur_ts, ExprId cur_lhs, const Node* list_node,
                                int& cursor) {
          if (!list_node || list_node->kind != NK_INIT_LIST) return;
          if (cursor < 0) cursor = 0;
          if (is_agg(cur_ts) && cur_ts.tag) {
            const auto sit = module_->struct_defs.find(cur_ts.tag);
            if (sit == module_->struct_defs.end()) return;
            const auto& sd = sit->second;
            size_t next_field = 0;
            while (cursor < list_node->n_children && next_field < sd.fields.size()) {
              const Node* item = list_node->children[cursor];
              if (!item) {
                ++cursor;
                ++next_field;
                continue;
              }
              size_t fi = next_field;
              if (item->kind == NK_INIT_ITEM && item->is_designated && !item->is_index_desig &&
                  item->desig_field) {
                for (size_t k = 0; k < sd.fields.size(); ++k) {
                  if (sd.fields[k].name == item->desig_field) {
                    fi = k;
                    break;
                  }
                }
              }
              if (fi >= sd.fields.size()) {
                ++cursor;
                continue;
              }
              const HirStructField& fld = sd.fields[fi];
              TypeSpec field_ts = field_type_of(fld);
              MemberExpr me{};
              me.base = cur_lhs;
              me.field = fld.name;
              me.is_arrow = false;
              ExprId me_id = append_expr(n, me, field_ts, ValueCategory::LValue);
              const Node* val_node = init_item_value_node(item);
              if (is_agg(field_ts) || field_ts.array_rank > 0) {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(field_ts, me_id, val_node, sub_cursor);
                  ++cursor;
                } else if (is_direct_char_array_init(field_ts, val_node)) {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                } else if (can_direct_assign_agg(field_ts, val_node)) {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                } else if (val_node && item->kind == NK_INIT_ITEM && item->is_designated &&
                           !item->is_index_desig && item->desig_field) {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                } else {
                  consume_from_list(field_ts, me_id, list_node, cursor);
                }
              } else {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(field_ts, me_id, val_node, sub_cursor);
                  ++cursor;
                } else {
                  append_assign(me_id, field_ts, val_node);
                  ++cursor;
                }
              }
              next_field = fi + 1;
            }
            return;
          }
          if (cur_ts.array_rank > 0 || is_vector_ty(cur_ts)) {
            if (cursor < list_node->n_children) {
              const Node* item0 = list_node->children[cursor];
              const Node* val0 = init_item_value_node(item0);
              if (is_direct_char_array_init(cur_ts, val0)) {
                append_assign(cur_lhs, cur_ts, val0);
                ++cursor;
                return;
              }
            }
            TypeSpec elem_ts = cur_ts;
            long long bound = 0;
            if (is_vector_ty(cur_ts)) {
              elem_ts = vector_element_type(cur_ts);
              bound = cur_ts.vector_lanes > 0 ? cur_ts.vector_lanes : (1LL << 30);
            } else {
              elem_ts.array_rank--;
              elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
              bound = cur_ts.array_size > 0 ? cur_ts.array_size : (1LL << 30);
            }
            long long next_idx = 0;
            while (cursor < list_node->n_children && next_idx < bound) {
              const Node* item = list_node->children[cursor];
              if (!item) {
                ++cursor;
                ++next_idx;
                continue;
              }
              long long idx = next_idx;
              if (item->kind == NK_INIT_ITEM && item->is_designated && item->is_index_desig) {
                idx = item->desig_val;
              }
              TypeSpec idx_ts{};
              idx_ts.base = TB_INT;
              ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
              IndexExpr ie{};
              ie.base = cur_lhs;
              ie.index = idx_id;
              ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
              const Node* val_node = init_item_value_node(item);
              if (is_agg(elem_ts) || elem_ts.array_rank > 0) {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
                  ++cursor;
                } else if (is_direct_char_array_init(elem_ts, val_node)) {
                  append_assign(ie_id, elem_ts, val_node);
                  ++cursor;
                } else if (can_direct_assign_agg(elem_ts, val_node)) {
                  append_assign(ie_id, elem_ts, val_node);
                  ++cursor;
                } else {
                  consume_from_list(elem_ts, ie_id, list_node, cursor);
                }
              } else {
                if (val_node && val_node->kind == NK_INIT_LIST) {
                  int sub_cursor = 0;
                  consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
                  ++cursor;
                } else {
                  append_assign(ie_id, elem_ts, val_node);
                  ++cursor;
                }
              }
              next_idx = idx + 1;
            }
            return;
          }
          if (cursor < list_node->n_children) {
            const Node* item = list_node->children[cursor];
            const Node* val_node = init_item_value_node(item);
            if (val_node && val_node->kind == NK_INIT_LIST) {
              int sub_cursor = 0;
              consume_from_list(cur_ts, cur_lhs, val_node, sub_cursor);
            } else {
              append_assign(cur_lhs, cur_ts, val_node);
            }
            ++cursor;
          }
        };
        int cursor = 0;
        consume_from_list(decl_ts, base_id, init_list, cursor);
      }
      return base_id;
    }
    case NK_NEW_EXPR: {
      TypeSpec alloc_ts = n->type;
      resolve_typedef_to_struct(alloc_ts);
      bool is_array = n->ival != 0;
      SizeofTypeExpr sot{};
      sot.type = qtype_from(alloc_ts, ValueCategory::RValue);
      TypeSpec ulong_ts{};
      ulong_ts.base = TB_ULONG;
      ExprId size_id = append_expr(n, sot, ulong_ts);
      if (is_array && n->right) {
        ExprId count_id = lower_expr(ctx, n->right);
        CastExpr cast_count{};
        cast_count.to_type = qtype_from(ulong_ts, ValueCategory::RValue);
        cast_count.expr = count_id;
        ExprId count_ulong = append_expr(n, cast_count, ulong_ts);
        BinaryExpr mul{};
        mul.op = BinaryOp::Mul;
        mul.lhs = size_id;
        mul.rhs = count_ulong;
        size_id = append_expr(n, mul, ulong_ts);
      }
      std::string op_fn;
      bool is_class_specific = false;
      if (!n->is_global_qualified && alloc_ts.base == TB_STRUCT && alloc_ts.tag) {
        std::string class_key = std::string(alloc_ts.tag) + "::";
        class_key += is_array ? "operator_new_array" : "operator_new";
        if (struct_methods_.count(class_key)) {
          op_fn = struct_methods_[class_key];
          is_class_specific = true;
        }
      }
      if (op_fn.empty()) op_fn = is_array ? "operator_new_array" : "operator_new";
      TypeSpec void_ptr_ts{};
      void_ptr_ts.base = TB_VOID;
      void_ptr_ts.ptr_level = 1;
      ExprId raw_ptr;
      if (n->left && n->is_global_qualified) {
        raw_ptr = lower_expr(ctx, n->left);
      } else {
        CallExpr call{};
        DeclRef callee_ref{};
        callee_ref.name = op_fn;
        TypeSpec fn_ptr_ts{};
        fn_ptr_ts.base = TB_VOID;
        fn_ptr_ts.ptr_level = 1;
        call.callee = append_expr(n, callee_ref, fn_ptr_ts);
        if (is_class_specific) {
          call.args.push_back(append_expr(n, IntLiteral{0, false}, void_ptr_ts));
        }
        call.args.push_back(size_id);
        if (n->left) {
          for (int pi = 0; pi < n->n_params; ++pi) {
            call.args.push_back(lower_expr(ctx, n->params[pi]));
          }
        }
        raw_ptr = append_expr(n, call, void_ptr_ts);
      }
      TypeSpec result_ts = alloc_ts;
      result_ts.ptr_level++;
      CastExpr cast{};
      cast.to_type = qtype_from(result_ts, ValueCategory::RValue);
      cast.expr = raw_ptr;
      ExprId typed_ptr = append_expr(n, cast, result_ts);
      if (n->n_children > 0 && alloc_ts.base == TB_STRUCT && alloc_ts.tag) {
        auto cit = struct_constructors_.find(alloc_ts.tag);
        if (cit != struct_constructors_.end() && !cit->second.empty()) {
          const CtorOverload* best = nullptr;
          if (cit->second.size() == 1) {
            best = &cit->second[0];
          } else {
            for (const auto& ov : cit->second) {
              if (ov.method_node->n_params != n->n_children) continue;
              best = &ov;
              break;
            }
          }
          if (best && !best->method_node->is_deleted) {
            LocalDecl tmp_decl{};
            tmp_decl.id = next_local_id();
            std::string tmp_name = "__new_tmp_" + std::to_string(tmp_decl.id.value);
            tmp_decl.name = tmp_name;
            tmp_decl.type = qtype_from(result_ts, ValueCategory::RValue);
            tmp_decl.init = typed_ptr;
            append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp_decl)}, make_span(n)});
            DeclRef tmp_ref{};
            tmp_ref.name = tmp_name;
            tmp_ref.local = LocalId{tmp_decl.id.value};
            ExprId tmp_id = append_expr(n, tmp_ref, result_ts, ValueCategory::LValue);
            CallExpr ctor_call{};
            DeclRef ctor_ref{};
            ctor_ref.name = best->mangled_name;
            TypeSpec ctor_fn_ts{};
            ctor_fn_ts.base = TB_VOID;
            ctor_fn_ts.ptr_level = 1;
            ctor_call.callee = append_expr(n, ctor_ref, ctor_fn_ts);
            ctor_call.args.push_back(tmp_id);
            for (int i = 0; i < n->n_children; ++i) {
              ctor_call.args.push_back(lower_expr(ctx, n->children[i]));
            }
            TypeSpec void_ts{};
            void_ts.base = TB_VOID;
            ExprId ctor_result = append_expr(n, ctor_call, void_ts);
            ExprStmt es{};
            es.expr = ctor_result;
            append_stmt(*ctx, Stmt{StmtPayload{std::move(es)}, make_span(n)});
            DeclRef ret_ref{};
            ret_ref.name = tmp_name;
            ret_ref.local = LocalId{tmp_decl.id.value};
            return append_expr(n, ret_ref, result_ts, ValueCategory::LValue);
          }
        }
      }
      return typed_ptr;
    }
    case NK_DELETE_EXPR: {
      bool is_array = n->ival != 0;
      ExprId operand = lower_expr(ctx, n->left);
      TypeSpec operand_ts = infer_generic_ctrl_type(ctx, n->left);
      std::string op_fn;
      bool is_class_specific = false;
      if (operand_ts.base == TB_STRUCT && operand_ts.tag && operand_ts.ptr_level > 0) {
        std::string class_key = std::string(operand_ts.tag) + "::";
        class_key += is_array ? "operator_delete_array" : "operator_delete";
        if (struct_methods_.count(class_key)) {
          op_fn = struct_methods_[class_key];
          is_class_specific = true;
        }
      }
      if (op_fn.empty()) op_fn = is_array ? "operator_delete_array" : "operator_delete";
      TypeSpec void_ptr_ts{};
      void_ptr_ts.base = TB_VOID;
      void_ptr_ts.ptr_level = 1;
      CastExpr cast{};
      cast.to_type = qtype_from(void_ptr_ts, ValueCategory::RValue);
      cast.expr = operand;
      ExprId void_operand = append_expr(n, cast, void_ptr_ts);
      CallExpr call{};
      DeclRef callee_ref{};
      callee_ref.name = op_fn;
      TypeSpec fn_ptr_ts{};
      fn_ptr_ts.base = TB_VOID;
      fn_ptr_ts.ptr_level = 1;
      call.callee = append_expr(n, callee_ref, fn_ptr_ts);
      if (is_class_specific) {
        call.args.push_back(append_expr(n, IntLiteral{0, false}, void_ptr_ts));
      }
      call.args.push_back(void_operand);
      TypeSpec void_ts{};
      void_ts.base = TB_VOID;
      ExprId del_call = append_expr(n, call, void_ts);
      ExprStmt es{};
      es.expr = del_call;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(es)}, make_span(n)});
      TypeSpec int_ts{};
      int_ts.base = TB_INT;
      return append_expr(n, IntLiteral{0, false}, int_ts);
    }
    case NK_INVALID_EXPR:
    case NK_INVALID_STMT: {
      TypeSpec ts{};
      ts.base = TB_INT;
      return append_expr(n, IntLiteral{0, false}, ts);
    }
    default: {
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID && n->kind != NK_CALL) ts.base = TB_INT;
      return append_expr(n, IntLiteral{0, false}, ts);
    }
  }
}

}  // namespace c4c::hir
