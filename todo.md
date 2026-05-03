# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after the `core.cpp`
migration. The first remaining compile boundary is now
`src/codegen/lir/hir_to_lir/expr/coordinator.cpp:68-69`, where member
function-pointer signature discovery still uses `base_ts.tag` to look up the
base aggregate layout in `mod_.struct_defs`. Same-wave residuals remain in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp` for LIR-owned `TypeSpec` tag
interning, aggregate type-ref helpers, object alignment, flexible-array global
lowering, and base-class type-ref construction.

## Suggested Next

Next coherent packet: migrate the narrow `expr/coordinator.cpp` member
function-pointer signature discovery route to structured aggregate owner
metadata or an existing named compatibility helper. Keep `hir_to_lir.cpp`
aggregate type-ref/flexible-array/base-class routes as the next separate
packet unless the coordinator fix needs a tiny shared helper.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- Keep the coordinator fix semantic: prefer owner metadata from `base_ts` and
  only use rendered spelling through explicitly named no-metadata compatibility
  fallback.
- Do not widen into `hir_to_lir.cpp` until the coordinator boundary is cleared;
  the `hir_to_lir.cpp` same-wave failures are numerous enough to deserve their
  own packet.

## Proof

Probe command:
`cmake --build --preset default > test_after.log 2>&1` with `const char* tag`
temporarily disabled failed at the `expr/coordinator.cpp` boundary above.

After reverting the temporary probe edit, `cmake --build --preset default`
passed. The accepted focused baseline remains in `test_before.log`; the current
`test_after.log` is the failed deletion-probe artifact and should be
overwritten by the next executor proof.
