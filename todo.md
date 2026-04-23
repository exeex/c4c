# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Migrate Float And Special-Case Return Support
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Continued step 2.4 by moving the reviewed `f128` address resolution, x87
load/store helpers, and long-double result publication path out of legacy
`f128.cpp` and into `lowering/float_lowering.cpp`. The reviewed float seam now
owns canonical `f128` load/store support used by float binops, unary negation,
and return-path loading, while `f128.cpp` is narrowed to cast/conversion
holdouts.

## Suggested Next

Continue step 2.4 by moving the remaining `f128` cast/conversion helpers out
of legacy `f128.cpp` into reviewed lowering owners, or explicitly classifying
which conversion holdouts must stay transitional, without pulling prepared or
module logic into the packet.

## Watchouts

- The reviewed float seam still relies on transitional `X86Codegen`
  declarations in `x86_codegen.hpp`; keep that shared surface unchanged for
  now.
- `alu.cpp` is now intentionally an i128-only dormant compatibility surface;
  do not move scalar float ownership back there just to preserve the legacy
  bucket.
- `return_lowering.cpp` already owns the live special-return path; the next
  packet should not re-home that policy back into `f128.cpp`.
- `f128.cpp` now mainly holds cast/conversion helpers plus shared local
  utilities; keep the next packet focused on that family instead of reopening
  load/store support.
- The reviewed scalar seam still relies on `X86Codegen` declarations in
  `x86_codegen.hpp`; keep that shared transitional surface unchanged for now.

## Proof

Step 2.4 float/f128 lowering seam follow-on on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
