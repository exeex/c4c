# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the
`TypeSpec::tag` deletion-probe blocker in
`src/frontend/hir/impl/expr/builtin.cpp::resolve_builtin_query_type`.
`FunctionCtx` now carries a parallel `tpl_bindings_by_text` map keyed by
parser template-parameter `TextId`, populated for function, method, and global
template lowering from the owning AST template parameter metadata.

`resolve_builtin_query_type` now checks `template_param_text_id` against that
TextId-keyed map before the existing module text-table lookup and no longer
reads `target.tag`. The stale rendered tag-only/no-metadata test still rejects
fallback, and a new focused test covers the important compatibility case where
the parser-owned template-param `TextId` is not decodable through
`module_->link_name_texts` but the structured TextId binding carrier is
available.

## Suggested Next

Continue Step 4 by rerunning the controlled `TypeSpec::tag` deletion probe.
Expected next first cluster should move past `builtin.cpp` and land in the
remaining HIR signature/callable/layout clusters such as `hir_functions.cpp`,
`hir_lowering_core.cpp`, `hir_build.cpp`, or `hir_types.cpp`.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Restore the worktree to a buildable state after recording probe failures.
- The first probe stopped in frontend/HIR compilation; later deletion probes
  may reveal additional parser/Sema/backend failures after these clusters are
  migrated or demoted.
- The parser `ast.hpp` helper cluster from the first probe is cleared for
  `encode_template_arg_debug_ref` and `typespec_mentions_template_param`;
  plain rendered template-parameter tag detection is no longer compatibility in
  that helper. `frontend_parser_lookup_authority_tests` now asserts this
  tag-only/no-carrier rejection.
- The HIR pending-type-ref display/debug cluster is cleared for
  `encode_pending_type_ref`; plain rendered tag spelling is intentionally not
  preserved there when the future no-tag field removal lands.
- The HIR canonical specialization-key display cluster is cleared for
  `canonical_type_str`; plain rendered tag spelling is intentionally not
  preserved there when the future no-tag field removal lands.
- The HIR layout TypeSpec lookup route is cleared for
  `find_struct_def_for_layout_type`; complete structured owner misses no longer
  fall back to rendered `struct_defs`.
- The HIR ABI/final-spelling mangling route is cleared for
  `type_suffix_for_mangling`; tag-TextId-only suffixes intentionally become
  deterministic id suffixes unless a record definition supplies final spelling.
- The HIR function specialization selector no longer matches tag-only nominal
  TypeSpecs after structured matching declines to decide.
- The HIR builtin layout route is cleared for
  `LayoutQueries::find_struct_layout`; complete structured owner misses no
  longer fall back to rendered `struct_defs`, and the no-metadata rendered
  `ts.tag` fallback has been intentionally removed. The sixth deletion probe
  did not report direct `find_struct_layout` `ts.tag` reads.
- The builtin layout full-suite regression is fixed by producer-side
  `record_def` / TextId metadata plus HIR-side translation from parser record
  nodes into module-owned owner keys. `LayoutQueries::find_struct_layout` still
  does not read `TypeSpec::tag`.
- `resolve_builtin_query_type` no longer reads rendered `TypeSpec::tag`.
  Parser-owned template-param TextIds that cannot be decoded from the HIR
  module text table now resolve through `FunctionCtx::tpl_bindings_by_text`.
  The string-keyed `tpl_bindings` remain compatibility for routes that can
  decode a module text name, while stale rendered tag-only/no-metadata tests
  still reject fallback.
- The HIR builtin query type route is cleared for
  `resolve_builtin_query_type`; `template_param_text_id` lookup is authoritative
  when present, and tag-only/no-metadata substitution is intentionally not
  preserved for the future no-tag field removal.
- The sixth deletion probe still stops in frontend/HIR compilation; no
  LIR/BIR/backend downstream metadata gap has been reached yet.
- Clusters cleared since the fifth probe: direct
  `LayoutQueries::find_struct_layout` fallback behavior and the full-suite
  layout/runtime regression. Since the sixth probe,
  `builtin.cpp::resolve_builtin_query_type` was also cleared. Remaining
  expected first-probe clusters are HIR callable/signature helpers in
  `hir_functions.cpp`, `hir_lowering_core.cpp` generic compatibility/layout
  helpers, `hir_build.cpp` ref-overload collection, `hir_types.cpp`
  typedef/layout/member/object inference helpers, and then call-lowering
  helpers in `impl/expr/call.cpp`.
- Deferred member typedef owner resolution now has a structured-derived
  instantiated-owner spelling bridge in `member_typedef.cpp` and
  `type_resolution.cpp`; rendered `tag` fallback remains explicit
  compatibility for no-carrier routes, and this is not yet a final structured
  owner-key carrier.
- `build_template_mangled_name` may still use rendered nominal `TypeSpec::tag`
  for struct/union/enum/typedef template arguments; add stale-rendered proof or
  migrate that helper before removing remaining deferred-member fallbacks.
- `hir_functions.cpp::substitute_signature_template_type` should now be
  retried as a separate bounded packet, with the member-owner signature tests
  kept in the focused proof set.
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

## Proof

Step 4 builtin query TextId binding proof was run with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(positive_sema_ok_expr_access_misc_runtime_c|positive_sema_ok_pragma_pack_c|cpp_positive_sema_alignas_applies_alignment_runtime_cpp|cpp_positive_sema_alignas_decl_storage_runtime_cpp|cpp_positive_sema_scoped_enum_underlying_layout_runtime_cpp|cpp_positive_sema_template_sizeof_cpp|cpp_hir_builtin_layout_query_sizeof_type|cpp_hir_builtin_layout_query_alignof_type|cpp_hir_builtin_layout_query_alignof_expr|llvm_gcc_c_torture_src_20071018_1_c|llvm_gcc_c_torture_src_pr34456_c|llvm_gcc_c_torture_src_stkalign_c|frontend_parser_lookup_authority_tests|frontend_parser_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and selected CTest passed 74/74. Focused coverage in
`frontend_hir_lookup_tests` proves the TextId-keyed binding route works even
when module text lookup cannot decode the parser TextId, while the stale
rendered tag-only/no-metadata route still rejects fallback. Proof log:
`test_after.log`.
