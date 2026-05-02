# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries reran the
`TypeSpec::tag` deletion probe after the `compile_time_engine` cleanup slices.
The temporary deletion in `src/frontend/parser/ast.hpp` was reverted before
this todo update, and the restored tree builds.

Third-probe first failure clusters:

- `src/frontend/hir/impl/expr/builtin.cpp::LayoutQueries::find_struct_layout`
  still falls back through `ts.tag` / `Module::struct_defs`; classify as HIR
  semantic layout lookup with rendered no-metadata compatibility.
- `src/frontend/hir/impl/expr/builtin.cpp::Lowerer::resolve_builtin_query_type`
  still resolves typedef template bindings by rendered `target.tag`;
  classify as HIR template-binding compatibility/key-format boundary.
- `src/frontend/hir/hir_functions.cpp` still has return/param substitution,
  member typedef return/param recovery, zero-sized struct return normalization,
  and pack binding fallback reads; classify as mixed HIR semantic and
  final-spelling compatibility.
- `src/frontend/hir/hir_lowering_core.cpp` still has generic type compatibility
  and record layout computation reads; classify as HIR semantic compatibility
  plus final layout storage payload.
- `src/frontend/hir/hir_build.cpp::collect_ref_overloaded_free_functions`
  still compares record args by rendered tags; classify as HIR semantic
  overload-signature compatibility.
- `src/frontend/hir/hir_types.cpp` still has typedef-to-struct, method owner,
  parity, base/member recovery, lower-struct base-tag payload, and expression
  inference reads; classify as mixed HIR semantic consumers plus
  display/final-spelling compatibility.
- `src/frontend/hir/impl/expr/call.cpp` now appears in the first-failure set
  for template-struct construction, pack expansion, member call, lower-call,
  and consteval-call template binding reads; classify as mixed HIR semantic
  call lowering and template-binding compatibility.

Cleared from the third-probe front line: all direct
`compile_time_engine.hpp` failures, including `type_suffix_for_mangling` and
`select_function_specialization`.

## Suggested Next

Continue Step 4 by choosing another compile-failure cluster and either
migrating it or explicitly demoting it to display/final-spelling/no-metadata
compatibility. Suggested next packet: migrate the narrow
`src/frontend/hir/impl/expr/builtin.cpp::LayoutQueries::find_struct_layout`
route so structured owner metadata is authoritative and rendered
`struct_defs` lookup is explicit no-metadata compatibility.

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

Step 4 third deletion probe ran:
`cmake --build --preset default` after temporarily disabling
`TypeSpec::tag` in `src/frontend/parser/ast.hpp`.

Result: build failed as expected in frontend/HIR compile. The temporary
`ast.hpp` deletion edit was reverted, then `cmake --build --preset default`
passed on the restored tree. `git diff --check` passed.
