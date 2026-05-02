# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries reran the controlled
`TypeSpec::tag` deletion probe after clearing the `builtin.cpp` layout
fallback. The temporary edit removed `const char* tag` from
`src/frontend/parser/ast.hpp`, ran `cmake --build --preset default` with
pipefail, captured the current first frontend/HIR compile-failure clusters, and
then restored `ast.hpp` to the pre-probe buildable state.

Fifth deletion-probe result: the build still stops in frontend/HIR compilation
before reaching LIR/BIR/backend. Cleared since the fourth probe:
`src/frontend/hir/impl/expr/builtin.cpp::LayoutQueries::find_struct_layout` no
longer appears as a direct `TypeSpec::tag` failure cluster, so `builtin.cpp`
is absent from the current first failure set. Current first remaining clusters
are:

- `src/frontend/hir/hir_functions.cpp`: mixed HIR semantic/template-binding and
  compatibility surfaces, including zero-sized struct return normalization,
  `substitute_signature_template_type`, callable return/parameter preparation,
  member typedef lookup, and pack binding name parsing.
- `src/frontend/hir/hir_lowering_core.cpp`: mixed semantic/display surfaces,
  including `generic_type_compatible`, local layout-size/layout-align helpers,
  and base-layout scratch TypeSpec construction.
- `src/frontend/hir/hir_build.cpp`: overload collection parity still compares
  record/union argument tags as rendered strings.
- `src/frontend/hir/hir_types.cpp`: broad HIR semantic/display cluster,
  including typedef-to-struct compatibility, struct method owner recovery,
  method lookup parity, layout fallback, inherited field/base construction,
  member symbol lookup, base tag storage/final spelling, generic type
  inference, template binding fallback, and member-call return recovery.
- `src/frontend/hir/impl/expr/call.cpp`: HIR call-lowering cluster for template
  struct calls, aggregate initializer parity, pack expansion, member calls,
  ordinary calls, synthetic `this` TypeSpec construction, and consteval call
  argument template binding fallback.

## Suggested Next

Continue Step 4 with one bounded frontend/HIR compile-failure cluster
migration. Suggested next packet: clear the `hir_functions.cpp`
`substitute_signature_template_type` / callable signature template-binding
surface by using existing structured type-parameter metadata before any
rendered binding-name compatibility, or explicitly demote the no-metadata
pieces needed for the future no-tag contract.

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

Step 4 fifth deletion probe ran with:
temporary removal of `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, then `cmake --build --preset default`.

Probe result: build failed as expected in frontend/HIR compile with the first
clusters classified above. The temporary `ast.hpp` deletion was restored, and
the restored tree passed `cmake --build --preset default`. `git diff --check`
passed after restoration.
