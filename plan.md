# FPR ABI Frame Policy And Placement Runbook

Status: Active
Source Idea: ideas/open/628_fpr_abi_frame_policy_and_placement.md

## Purpose

Separate FPR ABI and frame-placement work from scalar GPR ABI lowering by
requiring explicit prepared floating-point homes, result destinations, widths,
and save-slot placements before RV64 object emission consumes them.

## Goal

Make one or more representative FPR ABI/frame rows expose explicit prepared FPR
authority, or reclassify them to precise non-owned buckets with diagnostics.

## Core Rule

Do not treat FPR rows as GPR rows with different register names. RV64 consumers
must use explicit prepared FPR facts and fail closed when those facts are
missing, ambiguous, or outside this idea.

## Read First

- `ideas/open/628_fpr_abi_frame_policy_and_placement.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
- Current prepared call/result/frame planning and RV64 object-emission code
  around FPR arguments, FPR results, and FPR save/restore rows.

## Current Targets

- Representative rows named by the source idea:
  - `src/980605-1.c`
  - `src/ieee/compare-fp-2.c`
  - `src/ieee/unsafe-fp-assoc.c`
  - `src/pr39501.c`
  - nearby FPR-heavy stack-frame rows
- Prepared facts for FPR call arguments, call results, register homes, widths,
  result destinations, and frame save-slot placements.
- RV64 FPR consumers that currently reject or infer those facts.

## Non-Goals

- Scalar GPR call/result transport from idea 613.
- GPR dynamic-frame callee-saved placement from idea 626.
- Pointer stack-result policy from idea 627.
- Floating comparison semantics, floating casts, runtime mismatch triage,
  local/global producer repair, variadic/library policy, expectation changes,
  unsupported marker changes, allowlists, timeouts, or accounting.
- `f128` support.

## Working Model

- Evidence comes first. Identify the shared FPR authority gap before writing
  producer or consumer code.
- Existing prepared carriers should be reused when they already express the
  needed FPR facts. Add new authority only where an upstream producer owns it.
- Consumer admission should be narrow and fact-driven. It may unblock a
  representative row only when it proves the underlying prepared FPR authority.
- Reclassify rows that are not FPR ABI/frame authority into owner buckets
  instead of broadening this idea.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit the source idea unless durable intent changes or closure notes
  are required.
- Add focused producer or consumer tests for any code-changing step.
- Use `cmake --build --preset default` plus a supervisor-selected backend
  subset as the normal proof ladder for code slices.
- Treat expectation rewrites, unsupported-marker changes, testcase-shaped
  source matching, and final-assembly inference as route failures.

## Step 1: Refresh FPR ABI/Frame Evidence

Goal: identify the current shared FPR authority blockers across the
representative rows.

Actions:
- Re-run focused probes for the named FPR representative rows.
- Capture prepared dumps, RV64 object-route diagnostics, and relevant failure
  buckets.
- Distinguish FPR call argument/result authority, FPR frame save-slot
  placement, floating semantic gaps, local/global memory gaps, and runtime or
  unsupported-policy buckets.
- Record evidence and the next owner bucket in `todo.md`.

Completion check:
- `todo.md` lists the rows inspected, current prepared facts, current RV64
  rejection or emission point, and a recommended next step that is not
  named-case-only.

## Step 2: Trace FPR Producer Authority

Goal: locate the producers and carriers that should publish explicit FPR homes,
result destinations, widths, and frame placements.

Actions:
- Trace prepared call/result/frame planning for the Step 1 in-scope bucket.
- Identify whether existing prepared carriers can represent the needed FPR
  facts.
- Locate the first missing authority boundary, if any, before RV64 object
  emission.
- Record fail-closed conditions for missing, ambiguous, mismatched, or
  unsupported FPR facts.

Completion check:
- `todo.md` names the exact producer functions, carrier fields, consumer
  checks, and the smallest code-changing packet that can publish or consume
  explicit FPR authority.

## Step 3: Publish Or Verify Prepared FPR Facts

Goal: ensure prepared-layer facts are explicit for one in-scope FPR ABI/frame
family before RV64 lowering depends on them.

Actions:
- If upstream authority exists but is not published, add the narrow producer
  publication path.
- If publication already exists, add focused coverage proving the facts are
  complete before object emission.
- Keep FPR widths, register class, result homes, and frame placements explicit.
- Do not infer facts from source filenames, final stack layout guesses, or
  register order.

Completion check:
- Focused backend coverage proves the selected FPR facts are present in the
  prepared representation, or `todo.md` reclassifies the bucket with precise
  missing upstream authority.

## Step 4: Add Narrow RV64 FPR Consumer Admission

Goal: allow RV64 object emission to consume only explicit FPR facts for the
selected in-scope family.

Actions:
- Replace any broad FPR rejection or implicit inference for the selected family
  with validation of prepared FPR homes/placements.
- Emit only when register class, width, destination, and placement facts match
  the consumer contract.
- Add fail-closed tests for absent, malformed, mismatched, or unsupported FPR
  authority.
- Preserve existing scalar GPR and pointer stack-result behavior.

Completion check:
- Focused positive and negative backend tests pass, and the code path rejects
  FPR cases without explicit prepared authority.

## Step 5: Reclassify Representative Rows

Goal: determine whether the source idea is complete, needs another FPR packet,
or should split remaining work into separate initiatives.

Actions:
- Re-run the representative row probes after any Step 3 or Step 4 changes.
- Classify each remaining failure into FPR ABI/frame authority or an
  out-of-scope owner bucket.
- Record whether idea 628 is close-ready or which next FPR authority packet is
  justified.

Completion check:
- `todo.md` contains row-by-row classification, proof results, and a clear
  close/split/continue recommendation for the supervisor.
