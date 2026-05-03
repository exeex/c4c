# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after clearing the remaining
`hir_to_lir.cpp` boundaries. The first remaining compile boundary is now
`src/codegen/lir/hir_to_lir/lvalue.cpp:658-659`, where member field access
still falls back from structured identity to `access.base_ts.tag`. Same-wave
residuals also appear in `src/codegen/lir/hir_to_lir/types.cpp:59`,
`types.cpp:94-100`, and `types.cpp:171-177`, where field-chain layout helpers
still assign or recurse through `TypeSpec::tag`.

## Suggested Next

Next coherent packet: migrate the narrow `lvalue.cpp` member-access residual so
`access.tag` is recovered from structured owner metadata or an existing named
compatibility helper instead of `access.base_ts.tag`. Keep the `types.cpp`
field-chain residuals as the following packet unless the `lvalue.cpp` fix needs
a tiny shared helper.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` no longer contains `.tag` reads;
  remaining `struct_defs.find(...)` calls in that file use declaration-order or
  base-tag strings, not `TypeSpec::tag`.
- Avoid widening the next packet into `types.cpp`; its field-chain residuals
  are same-wave but distinct enough to handle after `lvalue.cpp`.

## Proof

Probe command:
`cmake --build --preset default > test_after.log 2>&1` with `const char* tag`
temporarily disabled failed at the `lvalue.cpp` boundary above.

After reverting the temporary probe edit, `cmake --build --preset default`
passed. The accepted focused baseline remains in `test_before.log`; the current
`test_after.log` is the failed deletion-probe artifact and should be
overwritten by the next executor proof.
