Status: Active
Source Idea Path: ideas/open/581_rv64_ordinary_floating_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Implement Ordinary Floating Cast Lowering

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by repairing RV64 object `CastInst` lowering for
ordinary F32/F64 immediate-source width casts.

Implementation facts:
- `fragment_for_prepared_floating_cast` now accepts an F32/F64 immediate
  operand when the cast is an ordinary `FPExt F32 -> F64` or
  `FPTrunc F64 -> F32` into an FPR home.
- The immediate path materializes the raw floating bits through scratch `t3`
  and an available scratch FPR, then emits the same `fcvt.d.s` or `fcvt.s.d`
  instruction used by FPR-register-source width casts.
- Existing FPR-register-source width casts, integer-to-FP casts, and
  FP-to-integer casts remain on their existing lowering paths.
- Unsupported types and unsupported homes still fail closed through the
  existing `unsupported_floating_cast` object-route diagnostic; no route
  allowlists, expectation rewrites, unsupported-marker changes, or filename
  checks were used.
- The focused constant F64-to-F32 `FPTrunc` fixture now builds an RV64 object
  and asserts materialized F64 bits, `fmv.d.x`, `fcvt.s.d`, `ret`, and no
  relocations.

## Suggested Next

Execute `plan.md` Step 4 by rerunning the representative routes from Step 1
and recording whether `920618-1.c`, `pr67218.c`, and `pr23941.c` now advance
past the prior `unsupported_floating_cast` owner, with F128/long-double rows
remaining quarantined.

## Watchouts

- Keep this lane limited to ordinary F32/F64 casts.
- If Step 4 still sees `unsupported_floating_cast`, distinguish unsupported
  ordinary F32/F64 source/destination shapes from intentionally quarantined
  F128/long-double/helper shapes before widening this route.
- The focused `UIToFP i32 -> F32` plus `FPExt F32 -> F64` chain was already
  supported before Step 3; any remaining `pr67218.c` issue is likely in the
  representative route context or a downstream owner rather than that minimal
  chain.
- Immediate floating-source materialization is now supported for ordinary
  F32/F64 width casts only.
- Do not use F128, long-double, soft-float helper, scalar compare, or variadic
  helper work as justification for this idea.
- Do not claim progress through unsupported-marker changes, expectation
  rewrites, route allowlist edits, named-case checks, or residual filename
  shortcuts.

## Proof

Proof output is in `test_after.log`.

- `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`: pass.
- Focused subset result: `backend_riscv_object_emission` passed, 1/1 tests.
