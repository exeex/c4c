# BIR Function Signature Semantic Producer Admission Runbook

Status: Active
Source Idea: ideas/open/561_bir_function_signature_semantic_producer_admission.md

## Purpose

Activate the function-signature producer lane split from the earlier combined
semantic admission route.

## Goal

Repair or prove BIR function-signature semantic producer admission at the real
return-info and parameter-layout boundaries, without routing the work into RV64
ABI or object emission before BIR signature facts are correct.

## Core Rule

Treat BIR function-signature fact publication as the owned surface. Do not
claim progress through expectation rewrites, unsupported downgrades, allowlists,
outer `latest function failure` note edits, scalar-control-flow repair,
scalar-binop repair, or downstream RV64 ABI/object work.

## Read First

- `ideas/open/561_bir_function_signature_semantic_producer_admission.md`
- `src/backend/bir/lir_to_bir/call_abi.cpp`
- The implementation of `infer_function_return_info()`
- Existing backend BIR semantic admission tests that cover function signatures,
  return info, parameter layouts, or fail-closed signature diagnostics.

## Current Scope

- Function return-info and parameter-layout producer behavior at the BIR
  signature boundary.
- Concrete parameter producer behavior in
  `lower_function_params_with_layouts()`.
- Representative RV64 function-signature rows, including `src/20050316-3.c`
  function `test1`, or a current stronger substitute.
- Nearby same-family rows where practical, including `src/20071029-1.c`,
  `src/ieee/pr72824-2.c`, `src/pr60960.c`, `src/pr70903.c`,
  `src/pr71626-1.c`, `src/pr71626-2.c`, `src/simd-6.c`, and
  `src/zero-struct-2.c`.

## Non-Goals

- Scalar-control-flow CFG, terminator, or phi-lowering repair.
- Scalar-binop instruction lowering repair.
- RV64 ABI or object-emission repair before BIR signature publication is
  proven correct.
- Classification-only, expectation-only, unsupported-marker, or allowlist
  changes.
- Broad call/ABI rewrites that leave the same BIR function-signature admission
  failure in place.

## Working Model

The common `latest function failure` module note is a failure publication
funnel, not the owned producer. Function-signature failures are produced before
block and instruction lowering when return-info inference or parameter layout
lowering fails. This runbook should isolate that producer boundary first, then
repair only confirmed signature fact publication gaps.

## Execution Rules

- Keep routine progress, representative diagnostics, and proof output in
  `todo.md`.
- Keep implementation changes narrowly tied to return-info or parameter-layout
  producer behavior.
- Add focused BIR coverage for semantic fact publication or fail-closed
  signature admission before claiming producer progress.
- If a representative advances to a downstream ABI or RV64 lowering boundary,
  record that owner boundary in `todo.md`; do not expand this plan into that
  downstream work.
- For code-changing steps, run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Step 1: Refresh Function-Signature Evidence

Goal: Determine the current function-signature producer boundary and whether
existing code already satisfies part of the source idea.

Actions:
- Inspect focused backend BIR tests for function-signature producer coverage.
- Re-run direct diagnostics for `src/20050316-3.c` function `test1`, or a
  current stronger function-signature substitute.
- Sample nearby same-family function-signature rows where practical, especially
  rows named in the source idea.
- Distinguish return-info failures, parameter-layout failures, outer module
  failure publication, and downstream ABI/RV64 owner boundaries.
- Run the delegated backend proof command and refresh `test_after.log`.
- Record the representative set, current owner boundary, and next packet
  recommendation in `todo.md`.

Completion Check:
- `todo.md` names the refreshed representative set and current producer or
  downstream owner boundary.
- `test_after.log` contains a fresh backend proof result.
- The next packet is either Step 2 for a real function-signature producer
  repair or Step 3 for broader validation and closure handoff.

## Step 2: Repair Function-Signature Producer Boundary

Goal: Fix only a confirmed remaining function-signature semantic producer
failure.

Actions:
- Work in the real signature producer path, including
  `infer_function_return_info()`, `lower_function_params()`, and
  `lower_function_params_with_layouts()`.
- Add or tighten focused BIR coverage for return-info or parameter-layout fact
  publication, or for explicit fail-closed owner-boundary rejection.
- Avoid named-case shortcuts for `src/20050316-3.c`, `test1`, or any single
  parameter/return shape.
- Prove the focused test and the backend subset.
- Update `todo.md` with the changed producer rule, proof command, and any
  downstream owner boundary discovered.

Completion Check:
- The original function-signature semantic admission diagnostic is repaired or
  replaced by an explicit fail-closed owner-boundary diagnostic at the real
  producer.
- Focused BIR coverage demonstrates the behavior.
- Backend proof is green, and no expectation/unsupported/allowlist downgrade
  was used.

## Step 3: Broader Validation And Closure Handoff

Goal: Decide whether the source idea is complete after refreshed evidence and
any needed producer repair.

Actions:
- Run the supervisor-delegated broader validation command, or at minimum the
  backend subset if no broader command is delegated.
- Confirm that remaining failures, if any, are downstream owner boundaries and
  not function-signature semantic producer failures.
- Record closure evidence and residual risks in `todo.md`.
- Ask the plan owner to decide whether to close, continue, or split a new open
  idea.

Completion Check:
- `todo.md` contains enough current proof for a close decision.
- The source idea is either ready for close gate review or has a concrete
  remaining function-signature producer packet.
