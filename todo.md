# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Migrate Float And Special-Case Return Support
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Started step 2.4 by standing up compiled reviewed owners
`lowering/float_lowering.cpp` and `lowering/float_lowering.hpp`, moving
canonical float binop, float-negation, and unary float dispatch out of the
dormant legacy `float_ops.cpp` and `alu.cpp` buckets, and wiring the new seam
into the backend target build. `float_ops.cpp` is now an explicit dormant
compatibility marker, while `alu.cpp` keeps only the honest later-step
`emit_copy_i128_impl` holdout.

## Suggested Next

Continue step 2.4 by moving the remaining `f128` load/store and special-return
helpers out of legacy `f128.cpp` and into reviewed `float_lowering` plus
`return_lowering` owners without pulling prepared or module logic into the
packet.

## Watchouts

- The reviewed float seam still relies on transitional `X86Codegen`
  declarations in `x86_codegen.hpp`; keep that shared surface unchanged for
  now.
- `alu.cpp` is now intentionally an i128-only dormant compatibility surface;
  do not move scalar float ownership back there just to preserve the legacy
  bucket.
- `f128.cpp` still owns long-double helpers and remains the next honest step-2.4
  migration target; do not mix its address/home support with prepared-route
  adapter work.
- The reviewed scalar seam still relies on `X86Codegen` declarations in
  `x86_codegen.hpp`; keep that shared transitional surface unchanged for now.

## Proof

Step 2.4 float lowering seam start on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
