Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Retained HIR Compatibility Paths

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory of retained HIR rendered-name
compatibility paths. This was an inventory-only packet; no implementation or
test files were changed.

Grep-friendly inventory:
- `src/frontend/hir/hir_ir.hpp`: `fn_index`, `global_index`,
  `lookup_function_id_by_legacy_rendered_name`,
  `lookup_global_id_by_legacy_rendered_name`,
  `find_function_by_rendered_decl_compatibility_name`,
  `find_global_by_rendered_decl_compatibility_name`,
  `find_function_by_name_legacy`, `find_global_by_name_legacy`,
  `resolve_function_decl`, and `resolve_global_decl`.
  Classification: metadata-rich plus explicit no-metadata compatibility. The
  structured indexes are `fn_structured_index` and `global_structured_index`;
  complete structured misses must not recover through rendered `fn_index` or
  `global_index`.
- `src/frontend/hir/hir_ir.hpp`: `struct_defs`,
  `struct_def_owner_index`, `find_struct_def_by_owner_structured`, and
  `struct_def_owner_matches_rendered`.
  Classification: metadata-rich with retained display/order rendered tags.
  The rendered `struct_defs` map is still the storage/layout map, while
  `struct_def_owner_index` is the structured authority.
- `src/frontend/hir/compile_time_engine.hpp`: `template_fn_defs_`,
  `template_struct_defs_`, `template_struct_specializations_`,
  `consteval_fn_defs_`, `enum_consts_`, `const_int_bindings_`, and their
  `*_by_key_` or `*_by_text_` mirrors.
  Classification: explicit no-metadata compatibility for rendered maps plus
  metadata-rich structured mirrors. `find_structured_node_entry`,
  `find_structured_vector_entry`, `find_structured_value_entry`, and
  `find_consteval_def(const Node*, ...)` already fail closed for complete
  structured misses.
- `src/frontend/hir/impl/compile_time/engine.cpp`:
  `PendingConstevalExpr::fn_name`, `pending_consteval_key`, and
  `find_pending_consteval_def`.
  Classification: `fn_name` is display/diagnostic and no-metadata fallback;
  `callee_identity` is the metadata-rich route and complete keys avoid
  rendered lookup.
- `src/frontend/hir/compile_time_engine.hpp`: `PendingTemplateTypeWorkItem`
  `identity_key` and `display_key`.
  Classification: `identity_key` is metadata-rich; `display_key` is
  display/order only and must not drive dedup/progress.
- `src/frontend/hir/impl/lowerer.hpp`: `FunctionCtx::locals`,
  `local_ids_by_text_id`, `rendered_compat_local_text_ids`,
  `rendered_compat_local_names`, `params`, `param_indices_by_text_id`,
  `rendered_compat_param_text_ids`, `rendered_compat_param_names`,
  `static_globals`, `static_global_ids_by_text_id`,
  `rendered_compat_static_global_text_ids`,
  `rendered_compat_static_global_names`, `label_blocks`,
  `local_const_bindings`, `local_const_bindings_by_text`, and
  `local_const_bindings_by_key`.
  Classification: metadata-rich local/param/static/global/const maps with
  explicit rendered compatibility mirrors; `label_blocks` is generated
  route-local lowering state.
- `src/frontend/hir/hir_types.cpp` and
  `src/frontend/hir/impl/expr/scalar_control.cpp`:
  rendered-compat lookup of local, param, static-global, and function-pointer
  signatures through `ctx.rendered_compat_*` and rendered maps.
  Classification: no-metadata compatibility bridges that must fail closed
  when structured local/param/static metadata exists and misses.
- `src/frontend/hir/impl/expr/object.cpp` and
  `src/frontend/hir/impl/stmt/decl.cpp`: constructor/member/aggregate owner
  recovery through `callee_name`, `resolve_member_lookup_owner_tag`,
  `rendered_typespec_tag_for_decl_compatibility`, `typespec_legacy_tag`, and
  rendered `module_->struct_defs`.
  Classification: mixed metadata-rich owner lookup plus no-metadata
  compatibility. These bridges must not silently retain stale rendered owner
  recovery after complete owner metadata misses.
- `src/frontend/hir/hir_functions.cpp`: signature/member-typedef helpers
  `apply_signature_template_binding_by_text`,
  `apply_legacy_template_binding_without_usable_text_spelling`,
  `member_typedef_compatibility_name`, and
  `find_struct_def_for_callable_type`.
  Classification: metadata-rich template/TextId lookup plus explicit
  no-complete-metadata compatibility.
- `src/codegen/shared/llvm_helpers.hpp` and
  `src/codegen/lir/hir_to_lir/core.cpp`: HIR-to-LIR aggregate owner/type
  handoff via `typespec_aggregate_owner_key`,
  `typespec_aggregate_final_spelling`,
  `typespec_aggregate_compatibility_tag`, `find_typespec_aggregate_layout`,
  `llvm_aggregate_value_ty`, and `lookup_structured_layout`.
  Classification: metadata-rich owner/layout lookup with no-metadata
  compatibility and final output spelling. Bridges must not use rendered tags
  as secondary authority for complete owner metadata misses.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: `dedup_globals`,
  `dedup_functions`, `finalize_module`, `collect_fn_refs`, and
  `eliminate_dead_internals`.
  Classification: metadata-rich `LinkNameId`/`TextId` preferred, with
  no-metadata compatibility for rendered names in dedup and LIR reference
  scans; final emitted spelling remains allowed output spelling.
- `src/codegen/lir/hir_to_lir/call/target.cpp`: `find_local_target_function`
  falls back from missing `LinkNameId` to `find_function_by_name_legacy`.
  Classification: no-metadata compatibility only; must not recover when a
  non-invalid `LinkNameId` misses.
- `src/codegen/lir/hir_to_lir/types.cpp`: field-chain lookup uses rendered
  `struct_defs` tags after structured `StructNameId` lookup context.
  Classification: HIR-to-LIR layout compatibility; must stay fenced to
  incomplete structured layout metadata.

Bridges that must not be silently retained:
- Complete `DeclRef` structured function/global misses recovering through
  rendered `fn_index` or `global_index`.
- Complete `HirRecordOwnerKey` misses recovering through rendered
  `struct_defs`, `typespec_legacy_tag`, `callee_name`, or compatibility tags.
- Complete `PendingConstevalExpr::callee_identity` misses recovering through
  `fn_name`.
- Complete local/param/static/global TextId or id misses recovering through
  `ctx.rendered_compat_*` name maps.
- Complete HIR-to-LIR aggregate owner/layout misses recovering through
  rendered compatibility tags.

## Suggested Next

Start `plan.md` Step 2 by fencing metadata-rich module declaration lookup in
`src/frontend/hir/hir_ir.hpp`: require complete structured
function/global misses in `resolve_function_decl` and `resolve_global_decl`
to fail closed before rendered `fn_index`/`global_index` compatibility, while
leaving explicit rendered-only callers routed through the named legacy helpers.

## Watchouts

- Do not weaken tests or mark supported HIR routes unsupported.
- Do not claim progress through rendered spelling expectation updates alone.
- Step 2 should not remove final output spelling or display/order storage.
- The likely first target is `hir_ir.hpp` module declaration lookup because it
  has structured indexes and explicitly named legacy helpers already present.
- Retained no-metadata bridges must fail closed for complete structured misses,
  especially owner/layout and consteval replay paths.

## Proof

No build proof required by the delegated packet. Inventory-only proof used
`rg`/`sed` inspection of HIR and HIR-to-LIR handoff surfaces; `test_after.log`
was not produced because no build or test command was delegated.
