# AArch64 00217 Runtime Mismatch Runbook

Status: Active
Source Idea: ideas/open/368_aarch64_00217_c_c_behavior_runtime_mismatch.md
Activated from: ideas/closed/367_semantic_bir_indirect_local_memory_lvalue_admission.md residual split

## Purpose

Localize and repair the post-semantic AArch64/runtime boundary now exposed by
`c_testsuite_aarch64_backend_src_00217_c`.

## Goal

Make the `00217` casted pointer-arithmetic unsigned update produce correct
runtime behavior after semantic BIR admission succeeds.

## Core Rule

Find the first incorrect post-semantic boundary and repair that general
backend/runtime capability. Do not claim progress through expectation changes,
unsupported classifications, runner behavior, timeout policy, proof-log
policy, CTest registration, filename-shaped matching, or output-text matching.

## Read First

- `ideas/open/368_aarch64_00217_c_c_behavior_runtime_mismatch.md`
- `todo.md`
- The closing context from idea 367, especially the fact that semantic
  `lir_to_bir` admission now succeeds for `00217`
- Current dumps or logs for semantic BIR, prepared BIR, generated AArch64, and
  runtime output for `c_testsuite_aarch64_backend_src_00217_c`

## Current Targets

- `c_testsuite_aarch64_backend_src_00217_c`: currently expected
  `data = "0123-5678"` but actually emits `data = "0123\x05"` after semantic
  handoff succeeds.
- The representative source shape: updating a global byte string through
  casted pointer arithmetic, `*(unsigned*)(data + r) += a - b`.
- Adjacent stability checks from the completed semantic owner:
  `backend_lir_to_bir_notes`, `c_testsuite_aarch64_backend_src_00005_c`, and
  `c_testsuite_aarch64_backend_src_00173_c`.

## Non-Goals

- Do not reopen semantic `lir_to_bir` indirect local-memory admission unless
  fresh evidence shows the exact old semantic handoff failure returned.
- Do not reopen the completed `00173` pointer-derived string-load/publication
  chain unless fresh evidence shows its old failure returned.
- Do not edit expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, proof-log policy, or external test
  contracts.
- Do not absorb broad ABI composite/byval/HFA/f128 work, variadic/floating
  work, dynamic stack work, aggregate writeback buckets,
  timeout/output-storm residuals, or unrelated AArch64 runtime mismatches.

## Working Model

Idea 367 moved `00217` past semantic admission. The active failure is now a
runtime output mismatch, so the owner may be prepared-BIR lowering,
pointer-derived address formation, cast-width load/store behavior, scalar
update value materialization, memory writeback, global string storage
publication, generated AArch64, ABI, or runtime execution. The first packet
must name the earliest incorrect boundary before a repair packet changes
behavior.

## Execution Rules

- Localize before repairing. Compare semantic BIR, prepared BIR, generated
  AArch64, and runtime output.
- Keep evidence tied to the first bad fact, not only to final output movement.
- Add focused backend or generated-code coverage when a small contract can
  represent the localized failing shape independently of `00217`.
- If direct focused coverage is not practical, record why in `todo.md` and use
  a stronger runtime or generated-code proof.
- Preserve the idea 367 stability checks while changing the localized
  post-semantic owner.
- Split a separate lifecycle idea instead of broadening this plan if evidence
  points to unrelated ABI, variadic, dynamic stack, aggregate writeback,
  timeout/output-storm, or separate runtime-mismatch work.

## Ordered Steps

### Step 1: Localize Post-Semantic Boundary

Goal: identify the first incorrect boundary that explains the
`data = "0123\x05"` runtime mismatch.

Concrete actions:

- Reproduce the current `00217` mismatch with the supervisor-delegated proof
  command.
- Capture or inspect semantic BIR, prepared BIR, generated AArch64, and
  runtime observations for the failing update.
- Compare the intended operation for `*(unsigned*)(data + r) += a - b` against
  the prepared memory operation, selected load/store width, update value, and
  emitted address sequence.
- Record in `todo.md` the first incorrect boundary, the candidate owner, and
  the evidence that rules out earlier boundaries.

Completion check:

- `todo.md` names a concrete prepared-BIR, AArch64, memory, ABI, or runtime
  owner, or explains why the plan must split before implementation.

### Step 2: Add Focused Coverage For The Localized Shape

Goal: protect the failing post-semantic capability outside the `00217`
filename when practical.

Concrete actions:

- Add or update focused backend/generated-code coverage for the localized
  owner, such as pointer-derived global-string address formation, cast-width
  scalar load/store, update-value materialization, or memory writeback.
- Assert the semantic or generated-code contract at the owner boundary rather
  than matching one emitted instruction neighborhood or the final
  `00217` output string.
- If no focused coverage is practical, document the reason in `todo.md` and
  name the stronger runtime/generated-code proof that will guard the repair.

Completion check:

- Focused coverage fails before the repair or the `todo.md` proof rationale
  clearly justifies using runtime/generated-code validation instead.

### Step 3: Repair The Localized Backend Or Runtime Capability

Goal: fix the general rule responsible for the first bad post-semantic fact.

Concrete actions:

- Change only the localized owner identified in Step 1.
- Preserve direct local-slot, completed `00173`, and idea 367 semantic
  admission behavior.
- Avoid special handling for `00217`, `data`, the exact string literal, one
  cast spelling, one offset, one generated temporary, or one emitted
  instruction neighborhood.
- Keep unrelated ABI, variadic, dynamic stack, aggregate, timeout, and runner
  behavior out of scope.

Completion check:

- The focused coverage or documented stronger proof passes, and `00217`
  advances beyond the current `data = "0123\x05"` mismatch or passes.

### Step 4: Validate Stability And Residual Ownership

Goal: prove the repaired owner and park any remaining first bad fact
correctly.

Concrete actions:

- Run the supervisor-delegated proof command after the repair.
- Confirm `backend_lir_to_bir_notes`, `00005`, and `00173` remain stable.
- If `00217` advances to a different post-semantic residual, record the new
  first bad fact in `todo.md` without expanding this plan.
- If the source idea is satisfied, leave enough proof detail in `todo.md` for
  close review.

Completion check:

- `todo.md` records proof results, adjacent stability status, and whether the
  source idea is ready for close review or needs a follow-up lifecycle split.
