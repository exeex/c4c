# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the HIR builtin
layout lookup route in
`src/frontend/hir/impl/expr/builtin.cpp::LayoutQueries::find_struct_layout`.
Complete structured owner metadata from `record_def` or `tag_text_id` plus
namespace/qualifier data is now authoritative: a structured owner hit returns
the metadata-backed layout, and a structured owner miss returns `nullptr`
instead of falling through to rendered `TypeSpec::tag` / `Module::struct_defs`.
Rendered `struct_defs` lookup remains only for no-metadata compatibility.

Focused coverage in `frontend_hir_lookup_tests` proves builtin
`sizeof`/`alignof` still prefer structured owner metadata over stale rendered
tags and now reject stale rendered layouts after a structured owner miss.

## Suggested Next

Continue Step 4 by choosing another compile-failure cluster and either
migrating it or explicitly demoting it to display/final-spelling/no-metadata
compatibility. Suggested next packet: migrate
`src/frontend/hir/impl/expr/builtin.cpp::Lowerer::resolve_builtin_query_type`
so template binding lookup uses existing structured type-parameter metadata
where available and rendered binding names remain an explicit compatibility
boundary.

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
  longer fall back to rendered `struct_defs`.
- The third deletion probe still stops in frontend/HIR compilation; no
  LIR/BIR/backend downstream metadata gap has been reached yet.
- Remaining first-probe clusters include HIR display/final-spelling helpers,
  HIR builtin/layout helpers, HIR call-lowering helpers, and mixed HIR semantic
  consumers in `hir_functions.cpp`, `hir_lowering_core.cpp`, `hir_build.cpp`,
  and `hir_types.cpp`.
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

Step 4 builtin layout proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and 62/62 selected tests passed. Proof log:
`test_after.log`. `git diff --check` passed.
