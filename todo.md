# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries reran the
`TypeSpec::tag` deletion probe after the recent parser/HIR cleanup slices. The
temporary deletion in `src/frontend/parser/ast.hpp` was reverted before this
todo update, and the restored tree builds.

Second-probe first failure clusters:

- `src/frontend/hir/compile_time_engine.hpp::type_suffix_for_mangling` still
  reads `ts.tag`; classify as HIR ABI/final-spelling/mangling payload, not
  semantic lookup. This is the best next bounded display/final-spelling packet.
- `src/frontend/hir/compile_time_engine.hpp::InstantiationRegistry::select_function_specialization`
  still has no-metadata specialization fallback comparisons on `spec_ts.tag` /
  `bind_ts.tag`; classify as HIR semantic compatibility fallback that needs a
  structured-first/no-metadata boundary.
- `src/frontend/hir/impl/expr/builtin.cpp` still has layout lookup and template
  binding fallback reads (`LayoutQueries::find_struct_layout`,
  `resolve_builtin_query_type`); classify as mixed HIR semantic/no-metadata
  compatibility.
- `src/frontend/hir/hir_functions.cpp`, `hir_lowering_core.cpp`,
  `hir_build.cpp`, and `hir_types.cpp` still contain first-pass failures for
  return/param substitution, generic type compatibility, layout computation,
  overload parity, typedef/member/base recovery, and lower-struct final
  base-tag payloads. Classify as mixed HIR semantic consumers plus
  display/final-spelling compatibility boundaries.

Old clusters now cleared from the first-probe front line: parser
`encode_template_arg_debug_ref` / `typespec_mentions_template_param`,
`encode_pending_type_ref`, `canonical_type_str`, and the structured-miss path
inside `find_struct_def_for_layout_type`.

## Suggested Next

Continue Step 4 by choosing another compile-failure cluster and either
migrating it or explicitly demoting it to display/final-spelling/no-metadata
compatibility. Suggested next packet: isolate
`compile_time_engine.hpp::type_suffix_for_mangling` behind a named
ABI/final-spelling compatibility helper that uses structured ids where
available and records the no-tag future tradeoff for tag-only inputs.

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
- The second deletion probe still stops in frontend/HIR compilation; no
  LIR/BIR/backend downstream metadata gap has been reached yet.
- Remaining first-probe clusters include HIR display/final-spelling helpers,
  especially ABI-sensitive `type_suffix_for_mangling`, HIR no-metadata
  specialization fallback comparisons, HIR builtin/layout helpers, and mixed
  HIR semantic consumers in `hir_functions.cpp`, `hir_lowering_core.cpp`,
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

Step 4 second deletion probe ran:
`cmake --build --preset default` after temporarily disabling
`TypeSpec::tag` in `src/frontend/parser/ast.hpp`.

Result: build failed as expected in frontend/HIR compile. The temporary
`ast.hpp` deletion edit was reverted, then `cmake --build --preset default`
passed on the restored tree. `git diff --check` passed.
