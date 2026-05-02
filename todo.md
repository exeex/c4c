# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the remaining
`TypeSpec::tag` compile blocker in
`src/frontend/hir/impl/expr/builtin.cpp::LayoutQueries::find_struct_layout`.
The builtin-local layout query now resolves record layouts only from structured
owner metadata derived from `record_def` or `tag_text_id` plus
namespace/qualifier data. If no complete structured owner carrier exists, or if
the structured owner misses, the helper returns `nullptr` and builtin
`sizeof`/`alignof` use their existing default layout size/alignment. The prior
rendered `ts.tag` / `Module::struct_defs` no-metadata fallback is intentionally
removed for the future no-tag contract.

Existing focused coverage in `frontend_hir_lookup_tests` continues to prove
structured builtin layout hits over stale rendered tags and structured misses
reject stale rendered layouts.

## Suggested Next

Continue Step 4 by rerunning the `TypeSpec::tag` deletion probe after the
builtin layout fallback removal, or choose the next first frontend/HIR
compile-failure cluster from the prior probe, likely a bounded
`hir_functions.cpp` template-binding/signature route or the
`hir_lowering_core.cpp::generic_type_compatible` rendered nominal comparison.

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
  `ts.tag` fallback has been intentionally removed.
- The HIR builtin query type route is cleared for
  `resolve_builtin_query_type`; `template_param_text_id` lookup is authoritative
  when present, and tag-only/no-metadata substitution is intentionally not
  preserved for the future no-tag field removal.
- The fourth deletion probe still stopped in frontend/HIR compilation before
  this slice; no LIR/BIR/backend downstream metadata gap has been reached yet.
- Remaining first-probe clusters after this slice should exclude
  `builtin.cpp`; rerun the deletion probe to confirm before selecting the next
  cluster. Expected remaining clusters include HIR call-lowering helpers and
  mixed HIR semantic/display consumers in `hir_functions.cpp`,
  `hir_lowering_core.cpp`, `hir_build.cpp`, and `hir_types.cpp`.
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

Step 4 builtin layout fallback proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and 62/62 selected CTest tests passed. Proof log:
`test_after.log`. `git diff --check` passed.
