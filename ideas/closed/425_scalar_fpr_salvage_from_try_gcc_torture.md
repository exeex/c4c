# Scalar FPR Salvage From try_gcc_torture

Status: Closed
Type: Salvage and rewrite planning idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: RV64/MIR scalar floating lowering
Reference Branch: `try_gcc_torture`

## Closure Summary

Closed after the scalar FPR salvage route produced exactly one ordered
bucket-backed follow-up idea:
`ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md`.

The accepted boundary is residual scalar F32/F64 cast object lowering for the
fresh `unsupported_floating_cast` rows `src/cvt-1.c`, `src/920618-1.c`, and
`src/pr66233.c`. The route records that older scalar residual rows
`src/ieee/930529-1.c` and `src/ieee/pr72824.c` are supporting context only.
F128/helper ABI/local-memory, aggregate/byval, stack-frame, and
`conversion.c`-only work are explicitly quarantined from this salvage output.

Close validation used matching full-suite `test_before.log` and
`test_after.log` with `3373` passed, `1` failed, and `3374` total before and
after. The single failure is the known stable `string_authority_guard`
baseline failure; regression guard passed with no new failures.

## Goal

Identify useful non-F128 scalar/FPR work from `try_gcc_torture` and rewrite it
as small, bucket-backed implementation ideas on reset `main`.

## Why This Exists

The exploratory branch contains useful scalar floating work mixed with a
low-value F128 route. We should not replay the branch wholesale, but we should
recover high-impact scalar F32/F64 lessons where failure buckets justify them.

## In Scope

- Review `try_gcc_torture` commits for scalar F32/F64 casts, scalar FPR binary
  ops, floating select/materialization, FPR local-store/reload, and branch
  guard handling.
- Cross-check each candidate against current RV64 gcc_torture bucket counts.
- Propose small rewrite ideas for high-frequency non-F128 scalar/FPR owners.
- Require focused tests and default CTest proof for every salvaged slice.

## Out Of Scope

- Cherry-picking the exploratory branch wholesale.
- F128 helper, F128 ABI, F128 local-memory, or conversion.c-driven work.
- Broad rewrites of `object_emission.cpp` without bucket evidence.
- Expectation, allowlist, unsupported-marker, or runtime-comparison changes.

## Acceptance Criteria

- A shortlist of salvage candidates is recorded with commit references,
  expected bucket impact, and rewrite risk.
- Low-value or F128-entangled candidates are explicitly rejected or quarantined.
- Follow-up ideas are ordered by broad non-F128 impact.
- Default CTest remains stable.

## Reviewer Reject Signals

- Reject direct cherry-picks that bring F128 dependencies along.
- Reject candidates justified only by `conversion.c`.
- Reject broad scalar/FPR rewrites without current bucket evidence.
- Reject tests that only prove a named gcc_torture row.
- Reject unsupported or expectation downgrades as progress.
