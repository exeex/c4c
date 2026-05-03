# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after the const-init
migration. The first remaining compile boundary moved to
`src/codegen/lir/hir_to_lir/core.cpp`: `record_structured_layout_observation`
still renders `observation.type_name` from `ts.tag`, `lookup_structured_layout`
still has legacy rendered-tag fallback lookup, and `lookup_abi_struct_layout`
still has the same legacy fallback. Same-wave residuals then appear in
`src/codegen/lir/hir_to_lir/expr/coordinator.cpp` member function-pointer
signature discovery and `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` aggregate
type-ref/flexible-array/base-class helper routes.

## Suggested Next

Next coherent packet: migrate the `core.cpp` first-boundary family so
structured layout observations, `lookup_structured_layout` compatibility
fallbacks, and ABI structured-layout lookup prefer HIR structured owner
metadata and only use named compatibility helpers for true no-metadata legacy
cases. Keep `hir_to_lir.cpp` and `expr/coordinator.cpp` as the next deletion
probe wave unless the core migration proves a tiny shared helper is needed.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- Treat `core.cpp` rendered-tag lookups as compatibility fallback boundaries,
  not as new semantic authority; prefer `tag_text_id`, `record_owner_name_id`,
  `is_template_instance`, and `type_args_key` when present.
- Do not widen the next packet into parser/Sema/HIR work. `hir_to_lir.cpp`
  aggregate type-ref helpers are a likely next packet after the core boundary.

## Proof

Probe command:
`cmake --build --preset default > test_after.log 2>&1` with `const char* tag`
temporarily disabled failed at the `core.cpp` boundary above.

After reverting the temporary probe edit, `cmake --build --preset default`
passed. The accepted focused baseline remains in `test_before.log`; the current
`test_after.log` is the failed deletion-probe artifact and should be
overwritten by the next executor proof.
