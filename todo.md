# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after the route review and
front helper migration. The first remaining compile boundary is now
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp:223`, where `object_align_bytes`
still falls back to `ts.tag` for aggregate layout. The same compile unit also
reports `hir_to_lir.cpp:467-468`, where flexible-array global lowering still
looks up `mod.struct_defs` through `ts.tag`. Same-wave residuals remain in
`src/codegen/lir/hir_to_lir/lvalue.cpp:658-659`, where member field access
falls back from structured identity to `access.base_ts.tag`.

## Suggested Next

Next coherent packet: migrate the two remaining `hir_to_lir.cpp` deletion-probe
boundaries in one narrow packet: `object_align_bytes` aggregate alignment and
flexible-array global lowering. Prefer existing structured layout helpers such
as `lookup_structured_layout`, `structured_layout_align_bytes`, and
`find_typespec_aggregate_layout` over direct rendered-tag lookup. Keep
`lvalue.cpp` member-access residual for the following packet unless this fix
naturally exposes a shared helper.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- The Step 5 route review at
  `review/step5_lir_aggregate_identity_route_review.md` judged the route
  aligned and recommended narrow probe-driven packets; no plan-owner rewrite is
  needed yet.
- Do not expand the next packet into broad global lowering or `lvalue.cpp`
  unless required by a tiny shared helper.

## Proof

Probe command:
`cmake --build --preset default > test_after.log 2>&1` with `const char* tag`
temporarily disabled failed at the `hir_to_lir.cpp` boundary above.

After reverting the temporary probe edit, `cmake --build --preset default`
passed. The accepted focused baseline remains in `test_before.log`; the current
`test_after.log` is the failed deletion-probe artifact and should be
overwritten by the next executor proof.

Baseline review:
rejected `test_baseline.new.log` for commit `fa4aaedc7`: the accepted
baseline is 3000/3000, while the candidate had 129 failures out of 3023.
The failed candidate log was deleted; keep `test_baseline.log` unchanged.
