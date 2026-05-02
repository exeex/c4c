# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries temporarily deleted
`TypeSpec::tag` from `src/frontend/parser/ast.hpp` and ran
`cmake --build --preset default`. The probe failed at compile time as
expected, and the temporary `ast.hpp` deletion was restored before this packet
ended. A follow-up `cmake --build --preset default` after restoration passed,
so implementation files are buildable again.

First failure clusters from the removal probe:

- Parser-owned semantic/display helpers in `src/frontend/parser/ast.hpp`:
  `encode_template_arg_debug_ref` still renders template argument debug text
  from `ts.tag`, and `typespec_mentions_template_param` still checks rendered
  template parameter names via `ts.tag`. Classification: parser/HIR-owned plus
  display/debug for encoding; rendered template-parameter compatibility for the
  mention check.
- HIR display/final-spelling helpers in `src/frontend/hir/hir_ir.hpp` and
  `src/frontend/hir/compile_time_engine.hpp`: `canonical_type_str`,
  `type_suffix_for_mangling`, and `encode_pending_type_ref` still need final
  spelling/debug/mangling text for TypeSpecs. Classification:
  display/final-spelling/mangling compatibility, not a semantic lookup
  blocker by itself.
- HIR template compatibility fallbacks in
  `src/frontend/hir/compile_time_engine.hpp::select_function_specialization`
  still contain rendered tag comparisons for no-structured-metadata TypeSpec
  argument matching. Classification: HIR-owned no-metadata compatibility
  surface already intentionally retained; next packet can isolate it behind an
  explicit compatibility helper or remove it when deletion is the active goal.
- HIR overload and type-route consumers in `src/frontend/hir/hir_build.cpp`
  and `src/frontend/hir/hir_types.cpp` still have direct `ts.tag` references:
  ref-overload struct receiver comparison, `resolve_typedef_to_struct`,
  `resolve_struct_method_lookup_owner_tag`, parity display comparison,
  layout fallback in `find_struct_def_for_layout_type`, inherited field/base
  traversal, member-symbol fallback, struct base recording/final spelling,
  generic control inference, template binding map lookup, and constructor/call
  type inference. Classification: mixed HIR-owned semantic consumers,
  no-metadata compatibility bridges, and final-spelling/base-tag payloads.
- No downstream LIR/BIR/backend failure appeared before frontend/HIR compile
  stopped. Classification: no downstream metadata-gap idea created in this
  packet; parser/HIR work remains owned by this runbook.

## Suggested Next

Continue Step 4 by choosing one compile-failure cluster and either migrating it
or explicitly demoting it to display/final-spelling/no-metadata compatibility.
A good next packet is to isolate display/final-spelling helpers
(`canonical_type_str`, `type_suffix_for_mangling`, `encode_pending_type_ref`,
or parser `encode_template_arg_debug_ref`) behind named compatibility
rendering, or to migrate a narrow HIR semantic consumer in `hir_types.cpp` such
as `resolve_typedef_to_struct` or `find_struct_def_for_layout_type` fallback
when existing `record_def`/TextId owner metadata is available.

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

Probe command:
`cmake --build --preset default`.

Result while `TypeSpec::tag` was temporarily deleted: build failed with
frontend/HIR compile errors beginning in `src/frontend/parser/ast.hpp`,
`src/frontend/hir/hir_ir.hpp`, `src/frontend/hir/compile_time_engine.hpp`,
`src/frontend/hir/hir_build.cpp`, and `src/frontend/hir/hir_types.cpp`.

Restoration proof: after restoring `src/frontend/parser/ast.hpp`,
`cmake --build --preset default` passed. `git diff --check` passed. No scratch
probe log was left behind.
