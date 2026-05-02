# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries attempted the bounded
`src/frontend/hir/hir_functions.cpp` signature/template-binding migration and
stopped on a real carrier boundary. A narrow prototype made
`substitute_signature_template_type` prefer `template_param_text_id` /
`tag_text_id`-derived binding keys and bridged callable template parameter
metadata from callable nodes into module TextIds. That repaired ordinary
template function signature substitution and parameter-pack expansion, but it
regressed existing deferred member-typedef signature cases such as
`cpp_hir_template_member_owner_signature_local`,
`cpp_hir_template_member_owner_field_and_local`, and
`cpp_hir_template_member_owner_decl_and_cast`.

Blocker: the failing member-typedef signature path still depends on rendered
owner/type spellings in `TypeSpec::tag` to seed or resolve
`typename wrapper<T>::alias` / `typename holder<T>::alias` style types. In this
file, existing structured carriers (`template_param_text_id`, `tag_text_id`,
`tpl_struct_origin_key`, and callable template parameter TextIds) are not
enough to recover the owner spelling needed by the deferred member-typedef
resolver without either keeping rendered `TypeSpec::tag` compatibility or
widening into the deferred member-typedef/template owner machinery outside the
owned route.

The unsafe prototype was reverted before return, leaving implementation files
buildable. Durable output for this packet is this blocker classification in
`todo.md`.

## Suggested Next

Continue Step 4 with one bounded frontend/HIR compile-failure cluster
migration, but do not retry `hir_functions.cpp` signature/member-typedef
handoff in isolation. Suggested next packet: migrate or add a structured owner
carrier in the deferred member-typedef/template owner route so
`typename wrapper<T>::alias` can be resolved without rendered `TypeSpec::tag`,
then return to `hir_functions.cpp::substitute_signature_template_type`.

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
- The fifth deletion probe still stops in frontend/HIR compilation; no
  LIR/BIR/backend downstream metadata gap has been reached yet.
- `builtin.cpp` is cleared from the current first deletion-probe failure set.
  Remaining first-probe clusters are HIR call-lowering helpers and mixed HIR
  semantic/display consumers in `hir_functions.cpp`, `hir_lowering_core.cpp`,
  `hir_build.cpp`, and `hir_types.cpp`.
- `hir_functions.cpp::substitute_signature_template_type` cannot be completed
  safely as a standalone slice because deferred member-typedef signature
  lowering lacks a structured owner spelling bridge in this file. The attempted
  local-only bridge regressed existing member-typedef HIR cases and was
  reverted.
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

Step 4 blocked-route restoration proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and 62/62 selected CTest tests passed after reverting the
unsafe prototype. Proof log: `test_after.log`. `git diff --check` passed.
