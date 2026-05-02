# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the
`lower_delete_expr` deletion-probe blocker in
`src/frontend/hir/impl/expr/object.cpp` while preserving the prior
`lower_new_expr` cleanup. Class-specific delete lookup now gates on a struct
pointer and lets `resolve_struct_method_lookup_owner_tag` resolve the owner
through structured metadata or its existing bounded compatibility path instead
of requiring a direct rendered `TypeSpec::tag` read.

## Suggested Next

Continue Step 4 by taking the next first deletion-probe blocker in
`src/frontend/hir/impl/inspect/printer.cpp`. The current probe has moved past
`lower_delete_expr`; residual direct `TypeSpec::tag` reads remain in
`inspect/printer.cpp`, followed by `expr/operator.cpp` and
`expr/scalar_control.cpp` surfaces.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- `hir_functions.cpp` no longer directly reads `TypeSpec::tag` in the targeted
  helper/member-typedef cluster.
- Qualified dependent member typedefs with a unique template owner/member are
  now resolved structurally from parser TextIds and current template bindings;
  keep `member_typedef_text_ids` as the member carrier and do not replace that
  with rendered owner-name lookup.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The callable zero-sized-return path should keep rendered `TypeSpec::tag`
  compatibility only when no complete structured owner metadata exists.
- The ref-overload grouping path no longer reads rendered `TypeSpec::tag`.
  It uses `record_def` and TextId-backed type-name identity before the bounded
  no-structured-metadata compatibility bridge.
- The member-symbol lookup path now treats complete owner/member metadata as
  authoritative; stale rendered owner symbols are not a fallback after a
  complete miss, but `HirStructDef` fields remain a structured hit source when
  the owner lookup mirror is absent.
- The generic `NK_MEMBER` inference path now delegates owner lookup to the
  layout resolver; do not reintroduce direct `struct_defs[TypeSpec::tag]`
  lookup when complete `record_def` or TextId owner metadata is present.
- The typedef-to-struct resolver now treats complete `record_def` and TextId
  owner metadata as authoritative; rendered `TypeSpec::tag` is retained there
  only through deletion-safe final-spelling/no-complete-metadata compatibility
  helpers.
- The struct-method lookup owner resolver no longer directly reads
  `TypeSpec::tag`; its remaining rendered fallback is a compatibility/display
  bridge through `tag_text_id` and the deletion-safe legacy payload helper.
- Struct-method return-type lookup parity no longer directly compares
  `TypeSpec::tag`; it now treats `record_def` and TextId owner metadata as the
  semantic identity and keeps legacy spelling only as the no-metadata
  compatibility comparison.
- `find_struct_def_for_layout_type` no longer directly reads `TypeSpec::tag`;
  its retained rendered lookup is a TextId-first/no-complete-metadata
  compatibility bridge.
- The TypeSpec overload of `find_struct_member_symbol_id` no longer falls back
  through `owner_ts.tag`; it uses rendered caller input first, then TextId-first
  compatibility spelling, then the deletion-safe legacy payload helper.
- Base-layout metadata in `lower_struct_def` no longer reads `base.tag`
  directly while populating `HirStructDef::base_tags`; `base_tags` remains
  rendered final spelling/compatibility storage, and `base_tag_text_ids` must
  only reuse TypeSpec TextId metadata after validating it against the chosen
  base tag.
- Base-layout metadata must not trust parser-owned TextIds through the HIR
  link-name text table unless the rendered text matches the chosen base tag.
  Keep the structured layout/`record_def` recovery before accepting a rendered
  compatibility spelling, or inherited-base lookup can silently point at an
  unrelated member name.
- `hir_lowering_core.cpp` no longer has direct `TypeSpec::tag` reads in generic
  record compatibility or local layout TypeSpec lookup.
- The base layout path still uses `HirStructDef::base_tags` as final spelling
  and as no-complete-metadata compatibility input; it no longer writes
  `TypeSpec::tag`.
- The former `hir_functions.cpp` lines 35-52 deletion-probe cluster is cleared
  of direct rendered tag reads.
- `hir_types.cpp` layout/member-base helpers now classify retained rendered
  lookup as no-complete-metadata compatibility. Base traversal should continue
  to prefer complete `base_tag_text_ids` owner metadata before rendered
  `base_tags`.
- The `hir_build.cpp` retained ref-overload deletion-probe failure is cleared.
- `hir_types.cpp` still contains lookup, parity, compatibility, and
  final-spelling payload surfaces outside the cleared layout/member-base helper
  family. Split those before editing so ABI/link-visible names, diagnostics,
  dumps, and base-tag storage are not accidentally collapsed into owner-key
  semantics.
- `hir/impl/expr/call.cpp` remains a later first-wave HIR cluster after the
  top-level HIR files compile.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Do not create downstream follow-up ideas until a probe reaches LIR/BIR/backend
  failures after frontend/HIR compile blockers are cleared.
- Classify each failure as parser/HIR-owned, compatibility/display/final
  spelling, or downstream metadata gap instead of fixing broad clusters inside
  the probe packet.
- Do not create new follow-up ideas for parser/HIR work that still belongs in
  this runbook.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Treat any `TypeSpec::tag` deletion build as temporary until Step 5.
- The `hir_lowering_core.cpp` and `hir_build.cpp` deletion-probe clusters are
  now clear of direct `TypeSpec::tag` reads. The first residual probe blocker
  moved past `resolve_struct_method_lookup_owner_tag`, the struct-method parity
  helper, `find_struct_def_for_layout_type`, the TypeSpec member-symbol
  fallback, and the base-layout metadata block to
  `src/frontend/hir/impl/expr/call.cpp:79`, followed by later `call.cpp`
  surfaces and then `src/frontend/hir/impl/expr/expr.cpp`.
- `infer_generic_ctrl_type` no longer directly reads `TypeSpec::tag`; its
  retained rendered-name compatibility is limited to deletion-safe final
  spelling and no-complete-metadata template-binding bridges.
- `call.cpp` no longer directly reads or writes `TypeSpec::tag`; its retained
  rendered-name compatibility is deletion-safe legacy spelling or final
  spelling only. Do not regress the newly TextId-first consteval and
  pack-forward template binding paths back to rendered tag lookup.
- The lvalue cast template-binding path in `expr.cpp` no longer directly reads
  `TypeSpec::tag`. Keep `template_param_text_id` and `tag_text_id` binding
  misses authoritative; do not fall back to rendered spelling when either
  structured carrier is present.
- The current deletion probe moved past `expr.cpp`,
  `try_lower_direct_struct_constructor_call`,
  `describe_initializer_list_struct`, the adjacent initializer-list member
  assignment owner fallback, and the later initializer/aggregate
  type-comparison cluster, plus the `lower_new_expr` allocation-owner and
  constructor lookup reads.
- The `lower_delete_expr` class-specific delete lookup no longer directly reads
  `TypeSpec::tag`; keep it routed through
  `resolve_struct_method_lookup_owner_tag` so structured owner metadata remains
  authoritative and the global delete fallback remains unchanged.
- The current deletion probe first reports direct `TypeSpec::tag` reads in
  `src/frontend/hir/impl/inspect/printer.cpp`, followed by
  `src/frontend/hir/impl/expr/operator.cpp`, and
  `src/frontend/hir/impl/expr/scalar_control.cpp`.
- Direct struct constructor lowering now treats rendered struct tags as
  final-spelling payload only. Keep its lookup path structured through
  `record_def`, template-origin metadata, and TextId/template binding carriers.
- Initializer-list struct description now treats `find_struct_def_for_layout_type`
  as the owner/layout authority. Keep `_M_array` and `_M_len` field discovery
  on the resolved `HirStructDef`; do not reintroduce direct
  `module_->struct_defs[TypeSpec::tag]` lookup for this path.
- Aggregate direct-assignment type comparison now treats structured nominal
  carriers as authoritative and rendered tags only as deletion-safe
  no-structured-metadata compatibility spelling. Do not turn this path back
  into direct rendered tag equality.
- `lower_new_expr` no longer directly reads `TypeSpec::tag`; keep both
  class-specific allocation lookup and constructor overload lookup on the
  single structured allocation-owner result.
- The rejected `ft.tag` layout repair route was replaced with a structured
  AST-node-to-HIR-owner carrier. Do not reintroduce rendered field type tag
  lookup for layout ownership.
- Non-canonical deletion probe artifacts for recent packets:
  `/tmp/c4c_typespec_tag_deletion_probe_step4_call_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object.log`, and
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object_next.log`.
  This packet also left
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object_new_expr.log` and
  `/tmp/c4c_typespec_tag_deletion_probe_step4_delete_expr.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(cpp_positive_sema_inherited_(base_aggregate_init|base_member_access|base_method_call|implicit_member_out_of_class|static_member_lookup_simple)_runtime_cpp|frontend_hir_lookup_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed, and CTest passed 78 of 78 delegated
tests, including the five inherited-base runtime regressions,
`frontend_hir_lookup_tests`, and all `cpp_hir_.*` tests.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_delete_expr.log 2>&1`, and restored
the temporary edit. The probe moved past both `lower_new_expr` and
`lower_delete_expr` in `src/frontend/hir/impl/expr/object.cpp`. The first
residual errors are direct `TypeSpec::tag` reads in
`src/frontend/hir/impl/inspect/printer.cpp`, followed by
`src/frontend/hir/impl/expr/operator.cpp` and
`src/frontend/hir/impl/expr/scalar_control.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
