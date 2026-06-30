# RV64 Integer Div And Rem Lowering

Status: Open
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
