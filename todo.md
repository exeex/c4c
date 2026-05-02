# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared one HIR
display/debug deletion-probe cluster in
`src/frontend/hir/compile_time_engine.hpp::encode_pending_type_ref`. Pending
type refs now encode structured debug payload ids (`tag_text_id`,
`template_param_text_id`, `deferred_member_type_text_id`, and
`record_def_text_id`) plus existing structural/final payloads (`base`, `ptr`,
`arr`, `origin`, template args, and deferred member name) instead of reading
rendered `TypeSpec::tag`.

Compatibility tradeoff: tag-only/no-metadata pending type refs no longer carry
the rendered tag spelling through this display/debug helper. That is the
intended no-tag future boundary for this packet; structural shape, origin,
template args, member-name compatibility, and TextId/record-def ids remain.

Focused coverage in `frontend_hir_lookup_tests` proves stale rendered tag
spelling is absent when structured metadata is present and no-metadata pending
type refs still retain explicit structural shape payload.

## Suggested Next

Continue Step 4 by choosing another compile-failure cluster and either
migrating it or explicitly demoting it to display/final-spelling/no-metadata
compatibility. A good next packet is to isolate the remaining HIR
display/final-spelling helpers (`canonical_type_str` or
`type_suffix_for_mangling`) behind named compatibility rendering, or to migrate
a narrow HIR semantic consumer in `hir_types.cpp` such as
`resolve_typedef_to_struct` or `find_struct_def_for_layout_type` fallback when
existing `record_def`/TextId owner metadata is available.

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
  that helper.
- The HIR pending-type-ref display/debug cluster is cleared for
  `encode_pending_type_ref`; plain rendered tag spelling is intentionally not
  preserved there when the future no-tag field removal lands.
- Remaining first-probe clusters include HIR display/final-spelling helpers,
  especially `canonical_type_str` and `type_suffix_for_mangling`, HIR
  no-metadata specialization fallback comparisons, and mixed HIR semantic
  consumers in `hir_build.cpp`/`hir_types.cpp`.
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

Step 4 delegated proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and 61/61 selected tests passed. Proof log:
`test_after.log`. `git diff --check` passed.
