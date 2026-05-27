# AArch64 HFA Aggregate Return Result Consumption Repair Runbook

Status: Active
Source Idea: ideas/open/57_aarch64_hfa_aggregate_return_result_consumption_repair.md
Switched From: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md

## Purpose

Route the non-owned first bad fact discovered during idea 56 Step 1 into its
own precise lifecycle owner.

## Goal

Repair AArch64 HFA aggregate call-result consumption so caller-side stores,
copies, and conversions consume the prepared ABI result lane registers after a
call, not stale allocated FPR homes.

## Core Rule

Use prepared call-result lane authority. Do not repair this route with
testcase names, rendered assembly filtering, local ABI reclassification, or
edge/terminator preservation changes.

## Read First

- `ideas/open/57_aarch64_hfa_aggregate_return_result_consumption_repair.md`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `tests/backend/bir/backend_prepare_liveness_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Scope

- HFA aggregate call-result lanes returned in AAPCS64 FP/SIMD result
  registers.
- Prepared `AfterCall` move bundles and ABI bindings for HFA result lanes.
- AArch64 caller-side consumers that store, copy, convert, or materialize those
  lanes after the call.
- Minimal prepared lookup or module-record plumbing needed to expose existing
  call-result lane facts to consumers.

## Non-Goals

- Do not implement idea 56 edge/terminator consumer preservation here.
- Do not change variadic register-save area layout, va_list cursor writeback,
  by-value argument publication, or general call staging unless tracing proves
  they are required for HFA return-result consumption.
- Do not rewrite callee-side return publication unless the callee no longer
  publishes the HFA ABI result lanes.
- Do not weaken tests or mark supported cases unsupported.

## Working Model

The observed failure is a call-result consumption bug: `fr_hfa12` publishes HFA
lanes to `s0` and `s1`, but the caller later stores or converts stale `s9` and
`s13`. Prepared liveness already expects HFA call-result lanes to appear as
distinct `AfterCall` ABI bindings. AArch64 lowering must preserve and consume
that authority through the caller-side aggregate-result uses.

## Execution Rules

- Treat `00204` and `fr_hfa12` as diagnostics only; implement a lane- and
  value-agnostic HFA call-result rule.
- Prefer existing `PreparedMoveBundle`, `AbiBindingRecord`,
  `CallResultRecord`, and value-home facts over raw BIR scans.
- If a new helper is needed, keep it narrow: answer which ABI result lane is
  authoritative for a post-call aggregate-result consumer.
- After each code-changing step, run a fresh build or compile proof plus the
  supervisor-selected focused subset.
- If tracing exposes another owner before an HFA return-result consumer bug,
  stop and classify it before expanding this plan.

## Ordered Steps

### Step 1: Reproduce And Trace The HFA Return-Result Consumer

Goal: Identify the exact caller-side consumer that uses stale FPR homes after
the HFA-returning call.

Primary targets:

- `c_testsuite_aarch64_backend_src_00204_c`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`

Actions:

- Re-run the supervisor-selected focused subset or inspect its existing proof
  for the `fr_hfa12()` mismatch.
- Trace the machine records emitted immediately after `bl fr_hfa12`.
- Identify whether stale `s9`/`s13` are selected by memory, cast, dispatch, or
  call-result publication lowering.
- Record the prepared facts that should have selected `s0`/`s1`.

Completion check:

- The stale consumer path and the missing prepared authority lookup are named
  before implementation proceeds.

### Step 2: Expose The Prepared HFA Result Lane Authority

Goal: Ensure consumers can ask for the ABI result lane source for each
post-call HFA aggregate-result value.

Primary targets:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/module/module.hpp`

Actions:

- Inspect existing `AfterCall` move-bundle, ABI-binding, call-result record,
  and value-home accessors.
- Reuse an existing lookup if it already maps the aggregate-result lane to the
  ABI source register.
- Add only the smallest missing structured lookup or record field if current
  consumers cannot reach the existing prepared lane facts.

Completion check:

- AArch64 lowering can identify `s0`/`s1` as the authoritative sources for the
  observed HFA result lanes without testcase names or raw BIR scans.

### Step 3: Consume ABI Result Lanes In Caller-Side Aggregate Uses

Goal: Make post-call stores, copies, and conversions consume the HFA ABI result
lanes instead of stale allocated homes.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`

Actions:

- Update the traced consumer path to prefer the prepared HFA call-result lane
  source when the use is tied to an aggregate result immediately after a call.
- Preserve existing scalar call-result and non-HFA aggregate behavior.
- Fail closed with a diagnostic if a required HFA result lane has no prepared
  ABI source.

Completion check:

- The caller-side sequence after `bl fr_hfa12` stores or consumes `s0`/`s1`
  for the returned lanes and no longer uses stale `s9`/`s13` for those values.

### Step 4: Prove Focused HFA Return-Result Behavior

Goal: Demonstrate the repaired route fixes the first bad fact without
downgrading nearby same-feature tests.

Primary target: supervisor-selected focused subset.

Actions:

- Run the exact build and focused proof command delegated by the supervisor.
- Include `c_testsuite_aarch64_backend_src_00204_c` and prepared HFA
  call-result lane tests.
- Inspect any remaining `00204` failure and classify it before making more
  changes.

Completion check:

- The observed `fr_hfa12()` mismatch is gone for semantic reasons, or a new
  first bad fact is classified to a precise owner.

### Step 5: Broader Regression Check And Handoff

Goal: Establish whether the HFA result-consumption repair is safe enough for
supervisor review and possible return to idea 56.

Primary target: repo-native broader backend validation chosen by the
supervisor.

Actions:

- Run the supervisor-selected broader check once the focused route is green or
  otherwise ready for review.
- Compare the implementation against the source idea's reviewer reject
  signals.
- If `00204` advances to an edge/terminator preservation first bad fact, record
  that evidence so the supervisor can decide whether to reactivate idea 56.

Completion check:

- Fresh proof exists, no expectation downgrade or testcase-shaped shortcut is
  present, and any remaining work has a precise lifecycle owner.
