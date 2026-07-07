Status: Active
Source Idea Path: ideas/open/581_rv64_ordinary_floating_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Focused Failing Coverage

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding focused RV64 object-emission coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` without changing
lowering behavior.

Coverage facts:
- Added an explicit retained-chain fixture for `UIToFP i32 -> F32` from GPR
  `t0` into FPR `ft0`, immediately followed by `FPExt F32 -> F64` into FPR
  `fs1`. This semantic chain currently builds and emits
  `fcvt.s.wu ft0, t0, rne; fcvt.d.s fs1, ft0, rne; ret`, so Step 3 should not
  treat that minimal home combination as a missing lowering target.
- Split the constant F64-to-F32 `FPTrunc` immediate-source case into its own
  focused precise rejection test. It still rejects with
  `unsupported_floating_cast`, making immediate floating-source materialization
  for ordinary FPR width casts the concrete Step 3 target.
- Kept existing FPR-register-source `FPExt`/`FPTrunc` positive coverage intact
  and avoided F128, long-double, helper-based casts, route allowlists, or
  residual testcase-name matching.

## Suggested Next

Execute `plan.md` Step 3 by teaching RV64 object `CastInst` lowering to
materialize ordinary F32/F64 immediate operands for width casts, starting with
constant F64-to-F32 `FPTrunc` into an FPR home, while preserving the current
fail-closed diagnostics for unsupported types and homes.

## Watchouts

- Keep this lane limited to ordinary F32/F64 casts.
- The focused `UIToFP i32 -> F32` plus `FPExt F32 -> F64` chain is already
  supported in the minimal GPR/FPR home combination; if representative routes
  still fail, investigate surrounding operands or downstream owners rather
  than reimplementing that exact chain.
- The current missing target is immediate floating-source materialization for
  FPR width casts, not FPR-register-source width casts.
- Do not use F128, long-double, soft-float helper, scalar compare, or variadic
  helper work as justification for this idea.
- Do not claim progress through unsupported-marker changes, expectation
  rewrites, route allowlist edits, named-case checks, or residual filename
  shortcuts.

## Proof

Proof output is in `test_after.log`.

- `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`: pass.
- Focused subset result: `backend_riscv_object_emission` passed, 1/1 tests.
