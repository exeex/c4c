# AArch64 Scalar Call Value Semantics

Status: Active
Source Idea: ideas/open/286_aarch64_scalar_call_value_semantics.md
Follows Closed Idea: ideas/closed/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md

## Purpose

Repair AArch64 backend scalar call argument and return-value lowering for
ordinary direct calls.

## Goal

Caller and callee agree on AAPCS64 scalar argument and scalar return-value
registers for ordinary direct calls, then prove the repair on focused backend
coverage and the clean runtime probes `00116.c` and `00159.c`.

## Core Rule

Fix the semantic call-value lowering rule. Do not special-case c-testsuite
files, function names, exact assembly text, expected outputs, unsupported
classifications, allowlists, timeout settings, or CTest behavior.

## Read First

- `ideas/open/286_aarch64_scalar_call_value_semantics.md`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `tests/c/external/c-testsuite/src/00116.c`
- `tests/c/external/c-testsuite/src/00159.c`
- `tests/c/external/c-testsuite/src/00100.c`
- `tests/c/external/c-testsuite/src/00121.c`

## Current Targets

- AArch64 direct-call scalar argument placement.
- AArch64 direct-call scalar return-value production and consumption.
- Focused backend tests plus `00116.c` and `00159.c` runtime probes.

## Non-Goals

- Do not repair variadic `printf`, string-literal addressing, loop predicates,
  short-circuit control flow, aggregate/pointer lowering, static storage,
  goto behavior, or macro/preprocessor-adjacent cases in this route.
- Do not weaken test contracts or runner behavior.
- Do not reclassify the whole 212-case scan in this route.

## Working Model

After LR preservation, `00100.c` and `00121.c` pass, but `00116.c` exits
quickly with `RUNTIME_NONZERO`. `00159.c` is a nearby quick mismatch involving
ordinary call values and broader call behavior. The next repair should prove
whether the AArch64 backend consistently places scalar arguments in ABI
registers and reads scalar return values from the ABI return register around
`bl`.

## Execution Rules

- Start with focused backend contracts for ordinary scalar direct calls before
  touching runtime probes.
- Prefer existing MIR/LIR/machine-node and call/return abstractions over ad hoc
  emitted instruction patches.
- Keep AAPCS64 register names and caller/callee responsibilities explicit in
  tests and implementation notes.
- Record `printf`, string-literal, loop, aggregate, short-circuit, static, and
  goto blockers as separate owners instead of expanding this plan.
- Run fresh build or compile proof before accepting any code-changing packet.

## Steps

### Step 1: Confirm Scalar Call-Value Failure Shape

Goal: Establish whether `00116.c` and nearby focused probes fail because scalar
arguments or scalar return values are not lowered through the AAPCS64 call ABI.

Primary target: AArch64 call and return lowering.

Actions:

- Inspect the current generated assembly or backend dumps for `00116.c` and
  `00159.c`.
- Trace where scalar call arguments are assigned to AArch64 argument registers.
- Trace where scalar return values are produced by callees and consumed by
  callers after `bl`.
- Check nearby AArch64 backend tests for existing scalar call-value contracts.

Completion check: `todo.md` names the owning implementation surfaces and the
proof command the executor should use for the first code slice.

### Step 2: Add Focused Scalar Call Tests

Goal: Create or update narrow backend tests that fail while caller/callee
scalar call values disagree.

Primary target: `tests/backend/mir/` AArch64 backend coverage.

Actions:

- Add focused direct-call expectations using existing test patterns.
- Assert scalar argument placement and scalar return-value use through backend
  facts or stable machine-printer output.
- Include at least one caller/callee shape that is not filename-specific to
  `00116.c` or `00159.c`.
- Keep runtime c-testsuite probes out of the unit-test contract unless the
  existing test harness already supports that shape.

Completion check: the new or updated focused test fails before the repair and
passes after the repair.

### Step 3: Repair Scalar Call Value Lowering

Goal: Implement ABI-consistent scalar argument and return-value lowering for
ordinary direct calls.

Primary target: AArch64 call and return lowering.

Actions:

- Derive scalar call argument and result locations from backend type/call facts
  rather than testcase names.
- Place ordinary scalar arguments in AAPCS64 argument registers at call sites.
- Ensure scalar returns are placed where callers expect them and consumed from
  the ABI return register after `bl`.
- Keep existing non-call and leaf return behavior stable unless the call-value
  contract requires a shared helper.

Completion check: focused backend tests pass and generated call-value assembly
for the probes no longer shows the old caller/callee value mismatch.

### Step 4: Prove Runtime Call Probes

Goal: Verify the repair removes the scalar call-value owner from the cleanest
runtime cases.

Primary target: `00116.c` and `00159.c` under the AArch64 backend runtime
route.

Actions:

- Run the supervisor-delegated narrow proof for the selected probes with
  explicit timeout behavior when using c-testsuite runtime routes.
- Confirm any remaining failure is not scalar argument or scalar return-value
  lowering.
- Record remaining semantic owners in `todo.md` instead of expanding this plan.

Completion check: the selected probes no longer fail because of scalar
call-value lowering, or `todo.md` records a different blocking owner with
evidence.

### Step 5: Reclassify Nearby Call Boundary

Goal: Decide what nearby call-related runtime cases remain after scalar
call-value lowering is repaired.

Primary target: the call-related subset surfaced by the LR timeout-boundary
rerun.

Actions:

- Rerun or inspect only the relevant call-related subset with explicit
  timeouts and stale-process cleanup if runtime tests are used.
- Separate cases fixed by scalar call-value repair from cases blocked by
  `printf`, string literals, variadics, loops, aggregates, globals,
  short-circuiting, or goto behavior.
- Create follow-on focused ideas only for semantic families that remain visible
  after ordinary scalar call values are no longer masking them.

Completion check: `todo.md` records the updated call boundary and any follow-on
idea paths.
