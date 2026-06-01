# AArch64 Prepared Consumer Wrapper Contraction

## Goal

Audit and thin the largest AArch64 prepared-authority consumer wrappers in
calls, memory, and ALU after ideas 69-72, without re-deriving authority or
moving target-local emission into shared code.

## Why This Exists

The major prepared-authority migrations succeeded, but the largest files remain
`calls.cpp`, `memory.cpp`, and `alu.cpp`. Some of that size is expected
AArch64 ABI, addressing, instruction selection, and record construction. Some
may now be wrapper layers that only adapt prepared facts to records and can be
merged, simplified, or moved to narrower local utilities.

## Owned Files

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- Existing AArch64-local helper owners only when they already own the helper
  shape.

## In Scope

- Build a helper inventory for prepared call-boundary, prepared memory access,
  prepared scalar ALU, and prepared scalar/control-flow consumers.
- Classify helpers as target-local emission, necessary prepared adapter,
  redundant wrapper, or possible existing-local-utility fold.
- Remove or merge redundant wrappers only after proving the prepared facts are
  still consumed directly.
- Keep ABI register conversion, concrete instruction spelling, scratch
  selection, addressing legality, and machine-record construction target-local.

## Out Of Scope

- Creating new BIR/prealloc authority unless a concrete missing fact is proven.
- Merging all large owner files mechanically.
- Dispatch relocation, edge-copy contraction, or machine-printer contraction.
- Test expectation weakening or unsupported-path downgrades.

## Proof Expectations

- Start with an audit table before implementation.
- For each implementation slice, run build proof plus focused backend tests for
  the affected owner family.
- Use regression guard before closure if more than one large owner changes.

## Reviewer Reject Signals

- Local code starts reconstructing prepared call, memory, or scalar authority
  under a renamed helper.
- AArch64 target-local emission details leak into shared prepared authority.
- A wrapper is deleted without proving equivalent diagnostics and machine
  records.
- The route becomes a broad line-count cleanup with no owner-specific proof.

## Closure Note

Closed on 2026-06-01 after the prepared consumer audit and wrapper contraction
runbook completed. The accepted route removed redundant ALU and memory prepared
wrapper surfaces while retaining call-boundary, store-publication, memory
adapter, and scalar compare/select helpers that own AArch64 ABI, addressing,
diagnostics, scratch choices, or machine-record construction. Reviewer handoff
found no testcase overfit, no expectation downgrade, and no prepared-authority
reconstruction under renamed helpers.

Closure proof used matching AArch64/backend-route before/after logs with 40/40
tests passing in both logs and `c4c-regression-guard` passing with
`--allow-non-decreasing-passed`.
