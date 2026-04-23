# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Migrate Float And Special-Case Return Support
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Continued step 2.4 by moving the remaining legacy `f128` cast/conversion
helpers out of `f128.cpp` and into `lowering/float_lowering.cpp`, including
the x87 `st(0)` and memory-entry integer conversion paths plus the shared
scalar float cast helpers they depend on. The reviewed float seam now owns the
legacy `f128` load/store and cast/conversion family, while `f128.cpp` is
reduced to an empty compatibility translation unit and special-return routing
remains under reviewed return lowering.

## Suggested Next

Review whether step 2.4 has any remaining float or special-return ownership
work beyond the still-transitional `X86Codegen` declarations in
`x86_codegen.hpp`; if not, hand the step back for lifecycle review instead of
reopening `f128.cpp`.

## Watchouts

- The reviewed float seam still relies on transitional `X86Codegen`
  declarations in `x86_codegen.hpp`; keep that shared surface unchanged for
  now.
- `alu.cpp` is now intentionally an i128-only dormant compatibility surface;
  do not move scalar float ownership back there just to preserve the legacy
  bucket.
- `return_lowering.cpp` already owns the live special-return path; the next
  packet should not re-home that policy back into `f128.cpp`.
- `f128.cpp` no longer owns active lowering logic in this family, so any
  follow-on should avoid repopulating it just to preserve the legacy bucket.
- The only remaining transitional surface in this packet boundary is the
  shared declaration layer in `x86_codegen.hpp`, not legacy conversion
  implementations.

## Proof

Step 2.4 float/f128 lowering seam follow-on on 2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
