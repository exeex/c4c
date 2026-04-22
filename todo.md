# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Migrate Scalar And Comparison Lowering Families
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed step 2.3 by moving the canonical compare/branch/select
implementation from the legacy top-level `comparison.cpp` bucket into compiled
reviewed owner `lowering/comparison_lowering.cpp` with matching
`lowering/comparison_lowering.hpp` seam wiring. The backend target now builds
comparison lowering from the new lowering translation unit, while
`comparison.cpp` remains only as an explicit dormant compatibility marker.

## Suggested Next

Keep step 2.3 focused on the remaining scalar-lowering family only: migrate the
next reviewed scalar owner into `lowering/scalar_lowering.*` without reopening
`alu.cpp` or expanding into adjacent prepared/render seams.

## Watchouts

- `comparison.cpp` is intentionally non-owning now; do not reattach live
  compare logic there just to preserve the old top-level bucket.
- The reviewed comparison seam still relies on `X86Codegen` declarations in
  `x86_codegen.hpp`; that transitional surface stays shared until a later
  lifecycle step moves declarations as well.
- Keep the remaining scalar migration isolated from `alu.cpp`,
  `scalar_lowering.*`, and other legacy buckets named in the do-not-touch set
  for this packet.

## Proof

Step 2.3 comparison lowering ownership move on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
