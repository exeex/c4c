# RV64 Integer Div And Rem Lowering

Status: Closed
Type: Implementation idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Source Evidence: `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
Owning Layer: RV64 integer instruction lowering
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Add semantic RV64 lowering for coherent scalar integer signed and unsigned
division and remainder fragments.

## Why This Exists

The regenerated RV64 gcc_torture scan classified 17
`unsupported_instruction_fragment` rows as integer div/rem lowering.
Representative rows include `src/20021120-3.c`, `src/20030105-1.c`,
`src/20090113-2.c`, `src/20090113-3.c`, and `src/20100416-1.c`;
`src/20021120-3.c` exposes a prepared scalar `bir.udiv` row.

## In Scope

- Lower coherent scalar `udiv`, `sdiv`, `urem`, and `srem` BIR operations for
  RV64-supported integer widths.
- Use type and signedness facts from prepared BIR instead of divisor or
  testcase shape.
- Add focused tests for signed division, unsigned division, signed remainder,
  and unsigned remainder.
- Preserve fail-closed diagnostics for unsupported widths or incomplete
  prepared operand/result facts.

## Out Of Scope

- Constant-divisor strength reduction or special handling for one divisor.
- Floating-point division, F128 helper work, or runtime helper policy.
- Pointer casts, select/join, call publication, aggregate ABI, or arithmetic
  shift lowering.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Acceptance Criteria

- Coherent scalar integer div/rem rows lower without
  `unsupported_instruction_fragment`.
- Signedness and result semantics are covered for both division and remainder.
- Unsupported or incoherent operand/result facts remain fail-closed.
- The proof includes focused div/rem tests and the supervisor-delegated
  regression subset.

## Reviewer Reject Signals

- Reject divisor-specific handling, width-specific shortcuts not grounded in
  the type system, or named-case fixes for `20021120-3.c` and nearby rows.
- Reject treating expectation rewrites, unsupported downgrades, or pass/fail
  accounting changes as div/rem capability progress.
- Reject lowering that ignores signedness or maps `sdiv`/`srem` through
  unsigned behavior.
- Reject helper renames or dispatch reshuffles that leave `bir.udiv`,
  `bir.sdiv`, `bir.urem`, or `bir.srem` rows failing with the same unsupported
  instruction-fragment mode.

## Completion Notes

Closed on 2026-06-30 as a bounded RV64 scalar integer div/rem lowering idea.

Completed focused coverage:

- `UDiv i32` lowers through RV64 `divuw`.
- `UDiv i64` lowers through RV64 `divu`.
- `SDiv i32` lowers through RV64 `divw`.
- `SDiv i64` lowers through RV64 `div`.
- `URem i32` lowers through RV64 `remuw`.
- `URem i64` lowers through RV64 `remu`.
- `SRem i32` lowers through RV64 `remw`.
- `SRem i64` lowers through RV64 `rem`.
- Missing prepared operand/result homes and pointer-typed div/rem forms remain
  fail-closed with `unsupported_instruction_fragment`.

Close-readiness probes under
`build/agent_state/430_step4_close_readiness/` show the source idea's
remaining representative failures are not unresolved div/rem target-lowering
work:

- `20100416-1` passes the RV64 object route with its `udiv i64` fragment.
- `20021120-3` still fails with mixed call, frame/computed call-argument,
  load, and store-local publication residuals.
- `20030105-1` still fails with select materialization, call, load, and
  store-local publication residuals.
- `20090113-2` still fails with select materialization, frame/computed
  call-argument, load, and store-local publication residuals.
- `20090113-3` still fails with select materialization, frame/computed
  call-argument, load, and store-local publication residuals.

Residual disposition:

- Call metadata, call-argument publication, load/local-store publication, and
  local/store producer facts remain under
  `ideas/open/422_bir_semantic_producer_high_impact_cleanup.md` unless a later
  plan owner splits a more precise implementation idea.
- Global-addressing and direct-global residual classes remain under
  `ideas/open/433_rv64_global_select_pointer_memory_residuals.md`.
- Select materialization remains a separate non-div/rem owner and must not be
  folded back into this closed idea.

Close proof:

- Existing canonical `test_before.log` and `test_after.log` cover the backend
  subset with `327 passed, 0 failed` before and after.
- The regression guard passed with non-decreasing pass-count semantics:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
