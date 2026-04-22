# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Migrate Scalar And Comparison Lowering Families
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed step 2.3 by moving the canonical integer-unary and integer-binop
implementation from the dormant top-level `alu.cpp` bucket into compiled
reviewed owner `lowering/scalar_lowering.cpp` with matching
`lowering/scalar_lowering.hpp` seam wiring and backend target build inclusion.
`alu.cpp` now remains only as an explicit dormant compatibility marker for the
honest non-scalar holdouts `emit_float_neg_impl` and `emit_copy_i128_impl`.

## Suggested Next

Close out step 2.3 by deciding whether the remaining explicit `alu.cpp`
holdouts belong in a dedicated non-scalar lowering seam or should stay parked
until the later i128/float-specific packets claim them.

## Watchouts

- `scalar_lowering.cpp` had two dormant compile debts from the old unbuilt
  `alu.cpp` body: its relative include path changed under `lowering/`, and
  `PhysReg` conflict checks must compare `.index` because the type has no
  equality operator.
- The reviewed scalar seam still relies on `X86Codegen` declarations in
  `x86_codegen.hpp`; keep that shared transitional surface unchanged for now.
- `alu.cpp` is intentionally non-owning for scalar logic now; do not move
  integer unary/binop ownership back there just to preserve the legacy bucket.

## Proof

Step 2.3 scalar lowering ownership move on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
