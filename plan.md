# Direct-Global Stack-Backed Pointer Branch Boundary Runbook

Status: Active
Source Idea: ideas/open/654_direct_global_stack_backed_pointer_branch_boundary.md

## Purpose

Repair the prepared/RV64 branch boundary where a pointer branch compares a
register-backed pointer with a direct-global stack-backed pointer operand.

## Goal

Advance the `src/20000314-3.c` direct-global pointer branch past
`unsupported_terminator_fragment`, or reclassify it to a precise missing
producer authority, without reopening previously closed branch families.

## Core Rule

Only consume a direct-global stack-backed pointer operand when direct-global
identity, pointer freshness, and branch stack-load authority are explicit.
Do not infer authority from stack offsets, source spelling, diagnostics,
assembly shape, or testcase identity.

## Read First

- `ideas/open/654_direct_global_stack_backed_pointer_branch_boundary.md`
- `ideas/closed/645_rv64_branch_residual_terminator_fragment_lowering.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

## Current Targets

- Primary testcase: `tests/c/external/gcc_torture/src/20000314-3.c`
- Expected current owner: first pointer branch after idea 645, comparing
  register `%p.varg0` with direct-global `@arg0`.
- Likely implementation surfaces:
  - prepared branch authority/publication code
  - RV64 branch operand consumption in object emission
  - focused backend tests covering direct-global stack-backed branch operands

## Non-Goals

- Do not reopen fused condition-plus-one-stack-operand branch lowering closed
  by idea 645.
- Do not reopen generic direct-global local-memory policy closed by idea 631
  unless refreshed evidence proves a new branch-specific producer gap.
- Do not reopen stack-carried pointer source publication for `%t6` or `%t23`.
- Do not change runtime policy, unsupported markers, expectation files,
  allowlists, timeouts, or pass/fail accounting.

## Execution Rules

- Keep each packet tied to one producer or consumer boundary.
- Preserve fail-closed diagnostics for missing direct-global identity, missing
  or ambiguous branch authority, stale pointer values, and unrelated terminator
  shapes.
- Add focused positive and negative coverage before accepting broad behavior.
- Use the supervisor-delegated proof command exactly for executor packets.
- Treat testcase-specific matching for `src/20000314-3.c`, `%p.varg0`, or
  `@arg0` as route drift.

## Step 1: Refresh Direct-Global Branch Evidence

Goal: Confirm the current branch shape and first owner before changing code.

Actions:
- Generate semantic BIR, prepared-BIR, ASM/object, and relevant diagnostics for
  `src/20000314-3.c`.
- Record whether the RHS direct-global operand has explicit stack-load
  authority and whether direct-global symbol identity is present at the branch
  consumer point.
- Identify whether the first missing fact is producer publication or RV64
  consumption.

Completion check:
- Evidence names the branch operands, direct-global identity state, branch
  stack-load authority state, and the current first fail-closed owner.

## Step 2: Publish Or Classify Direct-Global Branch Authority

Goal: Ensure prepared/prealloc facts represent the direct-global
stack-backed pointer branch shape explicitly when the producer can prove it.

Actions:
- If producer facts are incomplete, attach explicit direct-global branch
  operand authority only when symbol identity, freshness, and stack home
  evidence are complete.
- Keep missing, stale, ambiguous, non-direct-global, and unrelated branch
  shapes rejected with precise statuses.
- Add focused producer-side coverage for one legal shape and at least one
  missing-authority negative.

Completion check:
- Prepared evidence either publishes the direct-global branch authority for the
  legal shape or records a precise producer-side reason why no legal
  publication is available.

## Step 3: Consume Direct-Global Branch Authority In RV64

Goal: Let RV64 object emission lower the authorized direct-global
stack-backed pointer operand without guessing from stack or assembly shape.

Actions:
- Consume only the explicit prepared direct-global branch authority.
- Materialize or compare the direct-global operand according to the validated
  prepared facts.
- Preserve fail-closed behavior for missing, stale, ambiguous, or mismatched
  direct-global branch operands.
- Add focused RV64 coverage for the positive path and fail-closed negatives.

Completion check:
- The representative object route for `src/20000314-3.c` advances past
  `unsupported_terminator_fragment`, or fails closed at a more precise
  downstream owner backed by evidence.

## Step 4: Representative Integration And Regression Proof

Goal: Prove the branch boundary is complete without broad regressions.

Actions:
- Re-run the representative `src/20000314-3.c` object route and capture the
  new first owner if runtime still fails.
- Verify nearby direct-global and non-direct-global branch shapes remain
  guarded by explicit authority.
- Run the supervisor-selected backend proof command and preserve
  `test_after.log`.

Completion check:
- Backend regression proof shows no new backend failures.
- Any remaining failure is classified as downstream or outside this source
  idea, with no regression in direct-global branch authority.
