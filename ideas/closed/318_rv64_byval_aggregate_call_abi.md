# RV64 Byval Aggregate Call ABI

## Goal

Repair RV64 prepared call lowering for byval aggregate arguments so aggregate
payloads, aggregate addresses, and supported floating aggregate lanes are
transported according to the target call ABI instead of truncating emitted
assembly or faking callee-local state.

## Why This Exists

Idea 314 classified `src/00140.c` as a separate byval aggregate call ABI
residual. Step 5 evidence in
`build/rv64_c_testsuite_probe_latest/triage_314_step5/summary.md` shows emit
and link succeed, but qemu exits `132` and generated assembly is truncated at
`f1` byval aggregate handling. Prepared BIR still exposes byval aggregate-copy
loads through `%p.f`, while the simple prepared call emitter lacks the
caller-side aggregate-address and byval payload transport needed for this
shape.

## In Scope

- RV64 prepared call argument lowering for byval aggregate payloads.
- Caller-side aggregate-address and payload transport for byval arguments.
- Callee-side access to byval aggregate homes when supported by the call plan.
- Floating aggregate lane handling only where required for the narrow byval
  aggregate proof.
- Focused backend coverage for byval aggregate call semantics, including the
  shape exposed by `src/00140.c`.

## Out Of Scope

- Nested aggregate-local subobject stores/loads already repaired by idea 314.
- Vararg ABI repair beyond what is inseparable from a focused byval aggregate
  proof.
- Broad RV64 floating-point ABI rewrites outside byval aggregate calls.
- Function pointer, indirect-call, or external runtime-call policy.
- Fake callee-local copies that bypass caller-side payload transport.

## Acceptance Criteria

- Focused tests cover RV64 byval aggregate call argument transport and
  callee-side aggregate field access.
- `src/00140.c` either emits, links, and exits under qemu with status `0`, or
  any remaining failure is reclassified with concrete emitted-code evidence as
  a different mechanism.
- Repairs use prepared call plans, aggregate layout, and target ABI rules
  rather than filename, field-name, or stack-slot home matching.
- Assembly generation no longer truncates at the byval aggregate callee
  boundary for the supported proof case.

## Completion Note

Closed after Step 5 evidence in
`build/rv64_c_testsuite_probe_latest/triage_318_step5/summary.md` and
`probe_results.tsv`.

`src/00140.c` now passes the full RV64 probe:

- BIR dump: `0`
- prepared-BIR dump: `0`
- RV64 assembly emit: `0`
- clang link: `0`
- qemu: `0`
- classification: `supported-linked-pass`

The final route repaired byval payload staging, F32 lane handling, caller-side
pointer argument materialization, and callee incoming fixed GPR formal
publication. Fresh emitted `f1` records the byval address in its prepared stack
home, publishes incoming `%p.p` with `mv t0, a1`, and publishes incoming
`%p.n` with `mv s1, a2` before using prepared homes.

Focused positive coverage includes fixed byval calls, float-lane byval cases,
preserved pointer arguments, and formal GPR publication. The
`riscv64_byval_formal_gpr_publication` fixture uses a variadic declaration only
as a fixture-admission shape for fixed formal publication; it does not claim
broader vararg payload transport or `va_arg` lowering.

No further first-bad mechanism was exposed by the Step 5 reprobe, so no
follow-up idea was created.

Close-time guard: `cmake --build --preset default -j` passed, then
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
matched the rolled-forward `test_before.log` backend subset. The regression
guard passed with `--allow-non-decreasing-passed`: 271 passed, 1 accepted
existing failure (`backend_riscv_prepared_edge_publication`), 0 new failures.

## Reviewer Reject Signals

- The implementation teaches only `src/00140.c` or one `%p.f` spelling to pass
  instead of implementing a general byval aggregate call ABI path.
- The patch fakes callee-side aggregate values without moving the caller's
  aggregate payload through the call boundary.
- Runtime progress is claimed through expectation rewrites, qemu skips, or
  unsupported downgrades.
- Broad vararg or floating ABI rewrites are mixed in without focused byval
  aggregate proof and regression coverage.
- Assembly still truncates or silently omits byval aggregate payload transport
  behind a renamed helper or different failure mode.
