# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the bounded
`src/frontend/hir/hir_functions.cpp` callable/signature family starting at
`substitute_signature_template_type`.

The migrated family now treats `template_param_text_id` as authoritative for
signature template substitution when present. Callable parameter lowering first
uses `FunctionCtx::tpl_bindings_by_text`; the remaining string-keyed
`TypeBindings` lookup is only the final-spelling storage bridge reached from a
TextId-derived key, or the explicit compatibility path for no-TextId
TypeSpecs. Parameter-pack expansion likewise derives the pack base from
`template_param_text_id` when available before using the compatibility tag
bridge.

Callable return/parameter member-typedef preparation now resolves through
`record_def` / template-origin metadata before rendered owner names. The
zero-sized return normalization in the same callable family uses complete HIR
record-owner metadata before the rendered `struct_defs` bridge. Direct
`TypeSpec::tag` reads in this file are centralized in
`compatibility_type_tag`, which remains a compatibility/final-spelling bridge
for routes without structured metadata.

## Suggested Next

Continue Step 4 with the next bounded frontend/HIR deletion-probe cluster:
`src/frontend/hir/impl/expr/expr.cpp::is_ast_lvalue`, keeping structured
template-param and record metadata authoritative where present and leaving any
remaining rendered spelling reads explicitly classified as compatibility.

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
  preserved for the future no-tag field removal. The seventh deletion probe did
  not report this helper in the first failure clusters.
- The seventh deletion probe still stops in frontend/HIR compilation; no
  LIR/BIR/backend downstream metadata gap has been reached yet.
- Clusters cleared since the fifth probe: direct
  `LayoutQueries::find_struct_layout` fallback behavior and the full-suite
  layout/runtime regression. Since the sixth probe,
  `builtin.cpp::resolve_builtin_query_type` was also cleared. Remaining
  first-probe clusters are HIR lvalue/type-parameter binding compatibility in
  `impl/expr/expr.cpp`, callable/signature helpers in `hir_functions.cpp`,
  template-struct/member/consteval call helpers in `impl/expr/call.cpp`,
  constructor/aggregate/new-owner helpers in `impl/expr/object.cpp`,
  operator/member helpers in `impl/expr/operator.cpp`, and generic
  compatibility/layout/base-tag helpers in `hir_lowering_core.cpp`. Later
  clusters are still expected in `hir_build.cpp` ref-overload collection and
  `hir_types.cpp` typedef/layout/member/object inference helpers after the
  first parallel build failures are cleared.
- Deferred member typedef owner resolution now has a structured-derived
  instantiated-owner spelling bridge in `member_typedef.cpp` and
  `type_resolution.cpp`; rendered `tag` fallback remains explicit
  compatibility for no-carrier routes, and this is not yet a final structured
  owner-key carrier.
- `build_template_mangled_name` may still use rendered nominal `TypeSpec::tag`
  for struct/union/enum/typedef template arguments; add stale-rendered proof or
  migrate that helper before removing remaining deferred-member fallbacks.
- `hir_functions.cpp::substitute_signature_template_type` should now be
  considered migrated for the bounded callable/signature packet; remaining
  `TypeBindings` string-keyed access in this family is final-spelling storage
  after TextId lookup or no-metadata compatibility.
- `hir_functions.cpp` still has one centralized `compatibility_type_tag`
  bridge, and the controlled deletion probe still reports that helper as the
  remaining `hir_functions.cpp` compile blocker. Do not expand direct
  `TypeSpec::tag` reads back into semantic callsites.
- Supervisor reran the callable/signature proof with the checkout's actual HIR
  test names after the delegated regex matched no tests; the accepted proof is
  the build plus `frontend_hir_lookup_tests` and `cpp_hir_*`.
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

Accepted supervisor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved. The build passed;
CTest ran 73 HIR tests and all passed.

Regression guard fallback regenerated matching logs for the same command after
stashing this slice:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with before 73/73 and after 73/73.

Controlled deletion probe:

1. Temporarily removed `TypeSpec::tag` from
   `src/frontend/parser/ast.hpp`.
2. Ran `cmake --build --preset default`; result failed as expected during
   frontend/HIR compilation. The callable/signature callsites migrated by this
   packet no longer appear as separate failures, but
   `src/frontend/hir/hir_functions.cpp::compatibility_type_tag` remains the
   centralized compatibility bridge that still reads `TypeSpec::tag`.
3. Restored `src/frontend/parser/ast.hpp` exactly to the pre-probe state.
4. Ran `cmake --build --preset default`; result passed.
5. Ran `git diff --check`; result passed.
