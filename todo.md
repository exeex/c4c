# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries reran the controlled
`TypeSpec::tag` deletion probe after the builtin layout/query producer cleanup
and full-suite regression fix. The probe temporarily removed
`TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`cmake --build --preset default`, captured the current frontend/HIR
compile-failure clusters, then restored `ast.hpp` exactly and rebuilt the
normal tree.

Sixth-probe result: `LayoutQueries::find_struct_layout` remains cleared, but
`src/frontend/hir/impl/expr/builtin.cpp::resolve_builtin_query_type` is now the
first visible builtin.cpp blocker because its guarded template-param
compatibility bridge still reads `target.tag` after `template_param_text_id`
lookup misses in the HIR module text table. The next first-failure clusters are
HIR-owned signature/callable substitution in `hir_functions.cpp`, mixed
semantic/layout/final-spelling helpers in `hir_lowering_core.cpp`, function
overload collection in `hir_build.cpp`, and mixed semantic/display/layout
consumers in `hir_types.cpp`.

## Suggested Next

Continue Step 4 with one bounded HIR compile-failure cluster migration.
Suggested next packet: clear
`src/frontend/hir/impl/expr/builtin.cpp::resolve_builtin_query_type` by
replacing the guarded rendered template-param binding-name bridge with a
non-`TypeSpec::tag` payload, likely a TypeBindings-by-TextId bridge or a module
text-table attachment path for parser template-param TextIds.

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
- `resolve_builtin_query_type` still has a bounded rendered binding-name
  compatibility bridge for parser-owned template-param TextIds that cannot be
  decoded from the HIR module text table. This is guarded by
  `tag_text_id == template_param_text_id`; stale rendered tag-only miss tests
  still reject fallback. The sixth deletion probe now classifies this as the
  first builtin.cpp compile blocker for the future no-tag contract.
- The HIR builtin query type route is cleared for
  `resolve_builtin_query_type`; `template_param_text_id` lookup is authoritative
  when present, and tag-only/no-metadata substitution is intentionally not
  preserved for the future no-tag field removal.
- The sixth deletion probe still stops in frontend/HIR compilation; no
  LIR/BIR/backend downstream metadata gap has been reached yet.
- Clusters cleared since the fifth probe: direct
  `LayoutQueries::find_struct_layout` fallback behavior and the full-suite
  layout/runtime regression. Current first-probe clusters are
  `builtin.cpp::resolve_builtin_query_type`, HIR callable/signature helpers in
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

Sixth deletion probe proof:
1. Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`.
2. Ran `cmake --build --preset default`; result: expected probe failure in
   frontend/HIR compilation. First useful clusters:
   `src/frontend/hir/impl/expr/builtin.cpp::resolve_builtin_query_type`
   (`target.tag` guarded template-param compatibility),
   `src/frontend/hir/hir_functions.cpp`
   (`normalize_zero_sized_struct_return_from_body`,
   `substitute_signature_template_type`, callable return/param preparation,
   pack binding names),
   `src/frontend/hir/hir_lowering_core.cpp`
   (`generic_type_compatible`, local layout query helpers, base-tag layout
   construction),
   `src/frontend/hir/hir_build.cpp`
   (`collect_ref_overloaded_free_functions`), and
   `src/frontend/hir/hir_types.cpp` (typedef-to-struct, owner tag recovery,
   layout/member/object inference helpers), followed by
   `src/frontend/hir/impl/expr/call.cpp`.
3. Restored `src/frontend/parser/ast.hpp` exactly.
4. Ran `cmake --build --preset default`; result: restored tree builds.
5. Ran `git diff --check`; result: passed.

No extra root-level probe logs were left behind.
