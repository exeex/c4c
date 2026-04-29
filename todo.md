Status: Active
Source Idea Path: ideas/open/130_sema_hir_ast_ingress_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inspect HIR AST Ingress

# Current Packet

## Just Finished

Completed Step 2 HIR AST ingress audit.

Classification notes:

- Structured AST ingress: `make_ns_qual` builds `NamespaceQualifier` from
  AST `is_global_qualified`, `namespace_context_id`, and
  `qualifier_segments`, interning qualifier segment `TextId`s at
  `src/frontend/hir/hir_lowering_core.cpp:30`. The resulting qualifier is
  carried by `DeclRef`, `Function`, `GlobalVar`, and `HirStructDef` at
  `src/frontend/hir/hir_ir.hpp:374`, `src/frontend/hir/hir_ir.hpp:758`,
  `src/frontend/hir/hir_ir.hpp:822`, and `src/frontend/hir/hir_ir.hpp:876`.
- HIR `TextId` ingress: `make_text_id` and `make_unqualified_text_id` intern
  AST spellings, preferring `unqualified_name` before `name` at
  `src/frontend/hir/hir_lowering_core.cpp:46` and
  `src/frontend/hir/hir_lowering_core.cpp:51`. Primary consumers include
  function names and namespace qualifiers in `Lowerer::lower_function` at
  `src/frontend/hir/hir_functions.cpp:130`, consteval functions and params in
  `lower_non_method_functions_and_globals` at
  `src/frontend/hir/hir_build.cpp:516`, local declarations/references in
  `lower_local_decl_stmt` at `src/frontend/hir/impl/stmt/decl.cpp:33`, and
  variable references in `lower_var_expr` at
  `src/frontend/hir/impl/expr/scalar_control.cpp:155`.
- Structured HIR declaration authority: `ModuleDeclLookupKey` is built from
  `NamespaceQualifier` plus declaration `TextId` at
  `src/frontend/hir/hir_ir.hpp:189`; module indexing writes both rendered and
  structured function/global indexes at `src/frontend/hir/hir_ir.hpp:1304` and
  `src/frontend/hir/hir_ir.hpp:1311`. Resolution prefers concrete/link ids or
  structured lookup when available and records legacy rendered fallback as such
  in `resolve_function_decl`/`resolve_global_decl` at
  `src/frontend/hir/hir_ir.hpp:1501`, `src/frontend/hir/hir_ir.hpp:1528`,
  `src/frontend/hir/hir_ir.hpp:1617`, and
  `src/frontend/hir/hir_ir.hpp:1651`.
- Sema authority consumed by HIR: `build_initial_hir` stores the
  `ResolvedTypeTable` pointer in `Lowerer::resolved_types_` at
  `src/frontend/hir/hir_build.cpp:673` and
  `src/frontend/hir/impl/lowerer.hpp:124`. `fn_ptr_sig_from_decl_node` then
  uses `resolved_types_->lookup(n)` to override raw AST callable type recovery
  at `src/frontend/hir/hir_types.cpp:160`; parameters and locals consume this
  at `src/frontend/hir/hir_functions.cpp:430` and
  `src/frontend/hir/impl/stmt/decl.cpp:71`. Function return and struct-field
  callable signatures still canonicalize directly from AST declarators at
  `src/frontend/hir/hir_functions.cpp:157` and
  `src/frontend/hir/hir_types.cpp:1779`.
- String-keyed semantic lookup compatibility: HIR still keeps rendered-name
  maps for function/global indexes, enum/constexpr values, template defs,
  record defs, static members, member symbols, and struct methods. Important
  sites are `Lowerer` maps at `src/frontend/hir/impl/lowerer.hpp:1013`,
  `src/frontend/hir/impl/lowerer.hpp:1025`,
  `src/frontend/hir/impl/lowerer.hpp:1047`, and
  `src/frontend/hir/impl/lowerer.hpp:1063`. For record/member/method/template
  paths, structured by-owner maps are currently mirrors/parity checks rather
  than the returned authority; examples include
  `find_template_struct_primary` at
  `src/frontend/hir/impl/templates/templates.cpp:436`,
  `find_struct_method_mangled` at `src/frontend/hir/hir_types.cpp:548`,
  `find_struct_static_member_decl` at `src/frontend/hir/hir_types.cpp:1243`,
  and `find_struct_member_symbol_id` at `src/frontend/hir/hir_types.cpp:1287`.
- Legitimate generated/display/final text: link-visible/mangled names,
  specialization keys, static local names, temporary names, labels, asm text,
  diagnostics, and dumps are generated or display carriers rather than AST
  ingress authority. Examples are template/materialized names at
  `src/frontend/hir/hir_ir.hpp:1193`, `HirRecordOwnerKey::debug_label` at
  `src/frontend/hir/hir_ir.hpp:946`, static local globals at
  `src/frontend/hir/hir_types.cpp:1423`, temporary struct objects at
  `src/frontend/hir/impl/expr/scalar_control.cpp:68`, label lowering at
  `src/frontend/hir/impl/stmt/stmt.cpp:371`, asm string normalization at
  `src/frontend/hir/hir_lowering_core.cpp:62` and
  `src/frontend/hir/hir_lowering_core.cpp:101`, and HIR dump formatting under
  `src/frontend/hir/impl/inspect/printer.cpp`.
- Suspicious HIR follow-up targets: direct parsing of rendered
  `fn->name`/`n->name` still decides out-of-class method ownership and scoped
  static-member lookup at `src/frontend/hir/hir_lowering_core.cpp:147`,
  `src/frontend/hir/hir_build.cpp:487`, and
  `src/frontend/hir/impl/expr/scalar_control.cpp:86`. Template struct primary
  and specialization lookup still returns rendered-name results while only
  checking owner parity at `src/frontend/hir/impl/templates/templates.cpp:436`
  and `src/frontend/hir/impl/templates/templates.cpp:444`. Struct method,
  static member, and member-symbol lookups still return rendered tag/member
  lookups with owner-key parity at `src/frontend/hir/hir_types.cpp:548`,
  `src/frontend/hir/hir_types.cpp:584`,
  `src/frontend/hir/hir_types.cpp:620`,
  `src/frontend/hir/hir_types.cpp:1243`, and
  `src/frontend/hir/hir_types.cpp:1287`.

## Suggested Next

Proceed to Step 3: create focused follow-up idea(s) for the suspicious HIR and
Sema string-authority paths, especially rendered record/method/static-member
owner lookup and rendered template primary/specialization lookup.

## Watchouts

Do not collapse legitimate generated names into the same bucket as ingress
authority: mangled/link names, specialization keys, labels, diagnostics, dumps,
and final emitted names are valid output/display text. The highest-risk HIR
cleanup is not broad `std::string` removal; it is replacing rendered
record-owner/member/method/template lookup authority with the structured
owner/TextId keys that already exist as mirrors.

## Proof

Not run per delegated proof: read-only audit and `todo.md` update only; no
build proof or `test_after.log` required for this packet.
