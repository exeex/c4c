# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after the coordinator
migration. The first remaining compile boundary is now
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: LIR-owned `TypeSpec` tag interning,
field/global/signature aggregate type-ref helpers, object alignment,
flexible-array global lowering, and base-class type-ref construction still
read or assign `.tag`. Same-wave residuals also appear in
`src/codegen/lir/hir_to_lir/lvalue.cpp:658-659`, where member field access still
falls back from structured identity to `access.base_ts.tag`.

## Suggested Next

Next coherent packet: migrate the narrow front-of-file `hir_to_lir.cpp`
aggregate type-ref helper family (`lir_owned_type_spec`,
`lir_field_type_ref`, `lir_global_type_ref`, and `lir_signature_type_ref`) so
structured metadata or existing compatibility helpers supply aggregate refs
without direct `TypeSpec::tag` reads. Keep object alignment, flexible-array
global lowering, base-class type-ref construction, and the `lvalue.cpp`
same-wave residual for later packets unless this helper migration naturally
clears them.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- `hir_to_lir.cpp` line 39 is a carrier ownership/interning route; avoid
  replacing it with another rendered-string semantic key.
- The same-wave `lvalue.cpp` residual should not be bundled into the helper
  packet unless a shared helper is already required and the fix remains small.

## Proof

Probe command:
`cmake --build --preset default > test_after.log 2>&1` with `const char* tag`
temporarily disabled failed at the `hir_to_lir.cpp` boundary above.

After reverting the temporary probe edit, `cmake --build --preset default`
passed. The accepted focused baseline remains in `test_before.log`; the current
`test_after.log` is the failed deletion-probe artifact and should be
overwritten by the next executor proof.
