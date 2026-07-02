# Residual Scalar F32/F64 Cast Object Lowering

Status: Open
Type: Bucket-backed scalar FPR salvage implementation idea
Parent: `ideas/open/425_scalar_fpr_salvage_from_try_gcc_torture.md`
Owning Layer: RV64 prepared object lowering for scalar FPR/GPR conversion casts
Source Evidence:
- `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`
- `build/rv64_gcc_c_torture_backend/src_cvt-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_920618-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr66233.c/case.log`

## Goal

Lower or precisely reject the residual scalar F32/F64 cast shapes behind the
fresh `unsupported_floating_cast` RV64 gcc_torture rows without replaying
`try_gcc_torture` or broadening into F128/helper ABI work.

## Why This Exists

Idea 425 reviewed scalar-FPR salvage evidence from `try_gcc_torture` and found
exactly one accepted follow-up boundary with current-mainline bucket evidence:
the 2026-07-01 RV64 gcc_torture scan has three fresh
`unsupported_floating_cast` rows:

- `src/cvt-1.c`
- `src/920618-1.c`
- `src/pr66233.c`

The row diagnostics report that the RV64 object route currently supports only
prepared FPR width casts and I32/I64-to-F32/F64 integer-to-floating casts.
Older scalar residual rows such as `src/ieee/930529-1.c` and
`src/ieee/pr72824.c` are useful context, but they mix unrelated select,
local-byte, local-storage, `llvm.copysign.f32`, or branch-guard behavior and
must not drive this first scalar-FPR salvage slice.

## In Scope

- Preserve the exact fresh row evidence for `src/cvt-1.c`,
  `src/920618-1.c`, and `src/pr66233.c` before implementation.
- Identify each row's concrete cast opcode, source scalar type, destination
  scalar type, source bank, destination bank, source home, destination home,
  immediate/materialization needs, and current rejection site.
- Add semantic RV64 object lowering only for scalar F32/F64 cast directions
  whose prepared facts are explicit and coherent.
- Preserve existing accepted paths for prepared FPR width casts and
  I32/I64-to-F32/F64 integer-to-floating casts.
- Keep missing, contradictory, unsupported, or out-of-contract prepared facts
  fail-closed with owner-specific diagnostics.
- Add focused backend object-emission tests that construct representative
  prepared scalar-cast fixtures directly, plus fail-closed coverage for an
  unsupported scalar FP cast direction.
- Use the `try_gcc_torture` commits named by idea 425 as evidence to mine,
  not as patches to cherry-pick:
  `9d0f64883`, `b0700c2c3`, `88076c4ec`, `18c41c9c3`, `92d77cf2c`,
  `8bbaf8eb7`, and `f95ec37b9`.

## Out Of Scope

- F128 or long-double casts, helper calls, helper ABI, F128 lane pairs, F128
  local-memory work, and `conversion.c` completion.
- Aggregate/byval, stack-frame, parameter-home, or local-memory repairs.
- Scalar FPR binary arithmetic, floating branch guard lowering,
  select/materialization, FPR local-store/reload publication, same-module
  scalar FPR calls, and return move bundles.
- Inferring missing bank, type, home, opcode, or materialization facts inside
  RV64 object emission.
- Direct cherry-picks from `try_gcc_torture`, expectation rewrites, allowlist
  changes, unsupported-marker downgrades, runtime-comparison changes, or scan
  accounting changes.

## Acceptance Criteria

- The three fresh rows are inspected and either mapped to explicit scalar
  F32/F64 cast lowering contracts or split out when their first owner is
  missing prepared/BIR fact publication.
- Any implemented lowering is semantic across the supported scalar cast
  direction, not keyed to `src/cvt-1.c`, `src/920618-1.c`, `src/pr66233.c`,
  `conversion.c`, or old `try_gcc_torture` testcase names.
- Missing, contradictory, F128/helper-dependent, aggregate/byval,
  stack-frame, local-memory, branch/select, call/return, or local-store/reload
  cases remain fail-closed or become separate source ideas.
- Focused backend object-emission tests cover accepted scalar cast directions
  and adjacent unsupported directions.
- Focused representative probes for the three fresh rows show advancement past
  the old `unsupported_floating_cast` gate or a more precise owner-specific
  diagnostic.
- Acceptance validation includes a fresh build and the backend subset:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.

## Reviewer Reject Signals

Reject any route or slice that:

- uses `conversion.c`, a filename, or one of the three fresh row names as the
  semantic condition or primary proof
- directly cherry-picks broad `try_gcc_torture` behavior instead of rewriting a
  small current-mainline scalar-cast boundary
- changes expectations, allowlists, unsupported markers, runtime comparison,
  or scan accounting as progress
- admits F128/helper ABI/local-memory, aggregate/byval, stack-frame, scalar
  call/return, branch/select, or local-store/reload behavior under this scalar
  cast idea
- infers missing source/destination bank, scalar type, value home, cast opcode,
  or materialization authority in RV64 object emission
- only proves a named gcc_torture row and does not cover nearby supported and
  unsupported scalar cast directions with focused backend tests
- hides the old `unsupported_floating_cast` failure behind a new helper name
  without repairing semantic scalar F32/F64 cast lowering or producing a more
  precise owner-specific rejection
