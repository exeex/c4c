# AArch64 00204 Stdarg HFA Runtime Repair

## Goal

Repair the remaining AArch64 backend runtime mismatch in
`c_testsuite_aarch64_backend_src_00204_c`.

## Why This Exists

The latest pointer-carrier/frame-address drift repair restored the newly
regressed AArch64 c-testsuite cases:

- `c_testsuite_aarch64_backend_src_00032_c`
- `c_testsuite_aarch64_backend_src_00182_c`

The only remaining targeted failure is the pre-existing `00204.c` runtime
mismatch. Closure notes around the BIR/prealloc audits already identify this as
an AArch64 ABI-heavy case involving stdarg, variadic handoff, and HFA payload
behavior. It should be handled as its own semantic ABI repair, not mixed into
pointer-carrier cleanup.

## Owned Files

- Likely implementation:
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/variadic_entry_plans.cpp`
  - `src/backend/prealloc/regalloc/call_moves.cpp`
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/variadic.cpp`
- Likely tests:
  - `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  - `tests/backend/bir/backend_prepare_liveness_test.cpp`
  - relevant AArch64 backend route / c-testsuite proof only as needed

## In Scope

- Reproduce and characterize `00204.c` under AArch64 backend.
- Separate stdarg string/integer argument corruption from HFA float/double/long
  double payload corruption.
- Trace whether the first wrong fact originates in BIR call ABI, prealloc call
  plans, variadic entry plans, call moves, or AArch64 call lowering.
- Add focused contract coverage for the repaired ABI fact before changing broad
  c-testsuite expectations.
- Preserve the existing green coverage for `00032`, `00182`, and backend
  internal tests.

## Out Of Scope

- Rewriting the whole AArch64 calls owner layout.
- Downgrading `00204.c` expectations or marking it unsupported.
- Broad refactors of dispatch, memory, or unrelated prepared authority code.
- Fixing unrelated x86/RISC-V behavior.

## Proof Expectations

- Narrow before/after proof must include:
  - `c_testsuite_aarch64_backend_src_00204_c`
  - `c_testsuite_aarch64_backend_src_00032_c`
  - `c_testsuite_aarch64_backend_src_00182_c`
- Internal guard should include the relevant call/variadic/HFA contract tests.
- If `00204.c` is decomposed into smaller probes, each probe must name the ABI
  fact it proves and avoid testcase-shaped special casing.

## Closure Notes

Closed after `13587128e Repair AArch64 outgoing stack argument lifetime`.

The active repair identified the remaining runtime mismatch as an AArch64
outgoing stack argument lifetime issue for stack-passed arguments whose sources
are prepared frame slots. The final invariant publishes the x16-relative
outgoing argument slot as the destination lifetime while preserving the
selected prepared frame source bytes as uses, including byval register-lane and
HFA/FPR overflow stack materialization.

Focused proof passed `00204.c`, `00032.c`, `00182.c`, the prepared-frame stack
call contract, liveness guard, AArch64 call-boundary owner guard, byval stack
overflow route guards, HFA global payload route guard, and f128 HFA route guard.

Close-time backend guard used canonical `test_before.log` and `test_after.log`
for the same `^backend_` scope. Both logs report 178/179 passing with the same
known `backend_aarch64_instruction_dispatch` failure, and the regression guard
passes with non-decreasing passed count allowed: no new failing tests and no
pass-count loss.

## Reviewer Reject Signals

- The patch special-cases `00204.c` or its literal output shape.
- The repair only changes expectations.
- The route makes stdarg output less wrong while leaving the underlying call ABI
  fact unidentified.
- The proof omits the previously regressed `00032` and `00182` cases.
