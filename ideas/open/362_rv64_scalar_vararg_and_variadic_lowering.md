# RV64 Scalar Vararg and Variadic Function Lowering

Status: Open
Type: Target ABI follow-up
Parent: `ideas/closed/361_rv64_vararg_abi_hook_materialization.md`

## Goal

Complete the next RV64 variadic backend slice after target hook materialization:
scalar `va_arg`, `va_copy`, and the broader object-route variadic function
lowering gate.

## Why This Exists

Idea 361 closed the RV64 target hook/materialization milestone. Its Step 5
proof showed that representative cases no longer report the original target-only
missing facts for variadic entry state, `va_list` layout, `va_start` helper
resources/homes, or aggregate `va_arg` helper resources/homes.

The remaining failures are distinct:

- focused scalar `va_arg` missing facts
- focused `va_copy` missing facts
- representative torture cases blocked at the broader diagnostic:
  `RV64 object variadic function lowering is not implemented`

Those are larger lowering and runtime-state concerns, not part of the completed
target hook materialization acceptance criteria.

## In Scope

- Audit the current scalar `va_arg` and `va_copy` prepared facts that reach the
  RV64 target boundary.
- Define the RV64 lowering contract needed to support scalar `va_arg` and
  `va_copy` without source-shape inference.
- Materialize or precisely diagnose scalar `va_arg` homes and access plans.
- Materialize or precisely diagnose `va_copy` source and destination homes.
- Decide the first narrow object-route lowering step needed to move beyond the
  current RV64 variadic function admission gate.
- Add focused backend tests for scalar `va_arg`, `va_copy`, and any newly
  supported variadic function lowering path.
- Re-run representative RV64 allowlist cases once the focused lowering facts
  are in place.

## Out of Scope

- Reopening the completed target-hook acceptance criteria from idea 361 unless
  a real regression reintroduces the original missing facts.
- Testcase-name matching for representative torture cases.
- Unsupported expectation downgrades, skip broadening, or scan filtering to
  improve counts.
- Broad unrelated RV64 object-route features such as globals, vector ABI, FPR
  ABI, or generic instruction selection.
- Full support for every C vararg corner before focused scalar `va_arg`,
  `va_copy`, and admission-gate behavior is understood.

## Acceptance Criteria

- Focused backend tests show scalar `va_arg` facts are consumed by the RV64
  target route or rejected with precise target ABI diagnostics.
- Focused backend tests show `va_copy` facts are consumed by the RV64 target
  route or rejected with precise target ABI diagnostics.
- At least one narrow RV64 object variadic function lowering path moves past
  the current broad admission gate, or the gate is replaced with a narrower
  diagnostic tied to the exact unsupported lowering class.
- Representative allowlist outcomes for `src/20030914-2.c` and
  `src/920908-1.c` are rerun and documented.
- Existing baseline tests for touched backend/prepared surfaces remain green.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts or named-case-only fixes for scalar
  `va_arg`, `va_copy`, `src/20030914-2.c`, or `src/920908-1.c`.
- Reject unsupported expectation downgrades, skip broadening, or scan filtering
  claimed as RV64 variadic lowering progress.
- Reject helper renames, diagnostic text churn, or classification-only changes
  that leave the same scalar `va_arg`, `va_copy`, or variadic admission failure
  behind a new name.
- Reject RV64 object emission that guesses source-level vararg semantics instead
  of consuming explicit prepared vararg facts.
- Reject broad unrelated RV64 object-route rewrites presented as necessary for
  this focused vararg lowering slice.
- Reject retaining the exact broad `RV64 object variadic function lowering is
  not implemented` failure for all representative paths while claiming lowering
  progress.
