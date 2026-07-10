# RV64 20140828 Callee Result Frame-Slot Runtime Runbook

Status: Active
Source Idea: ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md

## Purpose

Repair the downstream RV64 runtime owner for `src/20140828-1.c` after the
completed pointer-branch source materialization chain from idea 653.

## Goal

Make the one-case RV64 runtime for
`tests/c/external/gcc_torture/src/20140828-1.c` match clang, or fail closed
with a precise unsupported diagnostic at the proven callee/result or
frame-slot value owner.

## Core Rule

Preserve the explicit `%t6` pointer-source publication and branch RHS
authority from idea 653. Do not re-infer the `%t6` source from stack offsets,
final assembly shape, source spelling, diagnostics, runtime behavior, or
testcase identity.

## Read First

- `ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md`
- `ideas/closed/653_stack_carried_pointer_source_publication_materialization.md`
- `build/agent_state/653_step4_representative_integration/summary.md`
- `docs/target_abi_contract_research/01_how_target_information_enters_the_pipeline.md`
- `docs/target_abi_contract_research/03_where_target_facts_are_split.md`
- `docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md`

## Current Targets

- Primary testcase: `tests/c/external/gcc_torture/src/20140828-1.c`
- Known completed boundary: semantic/prepared/RV64 publication and
  materialization of `%t6 = bir.add ptr %lv.a.0, 2` for the final branch RHS.
- Suspected downstream owner: callee return value propagation, `*d` frame-slot
  value preservation, or RV64 materialization around `f(a, 1, &d)`.
- Likely implementation surfaces:
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/prepared_lookups.cpp`
  - `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - focused backend/prepared tests for RV64 same-module call result and
    frame-slot value behavior

## Non-Goals

- Do not reopen stack-carried pointer source publication, branch RHS authority,
  or fused pointer branch terminator admission unless refreshed evidence proves
  a regression in those facts.
- Do not change runtime policy, unsupported markers, expectation files,
  allowlists, timeouts, or pass/fail accounting.
- Do not special-case `src/20140828-1.c`, `%t6`, `f(a, 1, &d)`, `&a[1]`, or
  `d != 1`.
- Do not claim progress through helper renames, diagnostic-only rewrites, or
  classification-only changes that leave the same qemu abort.

## Working Model

`main` initializes `a[0]`, calls `f(a, 1, &d)`, then checks both the returned
pointer and the stored value in `d`. Idea 653 proved the final branch RHS
address for `&a[1]` is explicit and materialized. The remaining abort must be
localized to the call path before that branch: the callee result pointer, the
callee store through `d`, or the caller-visible frame-slot/value-home state
after the call.

## Execution Rules

- Start every code-changing packet from fresh BIR, prepared-BIR, ASM, object,
  disassembly, and qemu evidence for the one case.
- Identify the first wrong value before changing lowering.
- Repair only the general callee/result or frame-slot value rule proven by the
  evidence.
- Preserve fail-closed diagnostics for missing, incomplete, stale, ambiguous,
  or mismatched prepared facts.
- Add focused positive and negative coverage before treating the runtime case
  as proof.
- Use the supervisor-delegated proof command exactly for executor packets.

## Step 1: Refresh 20140828 Runtime Evidence

Goal: Confirm the current first wrong-value boundary after idea 653.

Actions:
- Generate semantic BIR, prepared-BIR, ASM, object, disassembly, and runtime
  evidence for `src/20140828-1.c`.
- Compare clang and c4c RV64 behavior under qemu.
- Trace the values relevant to `f(a, 1, &d)`: the incoming `a` pointer, the
  returned `a + 1` pointer, the incoming `d` address, the callee `*d = c`
  store, and the caller reload/check of `d`.
- Verify that `%t6` branch source publication, prepared branch authority, and
  RV64 branch materialization remain present.

Completion check:
- Evidence names the first divergent value or store/load boundary before the
  abort, and confirms whether the owner is call result propagation,
  frame-slot value preservation, indirect store materialization, or another
  precise downstream route.

## Step 2: Prove The Owner And Add Focused Contract Coverage

Goal: Turn the Step 1 boundary into a focused prepared/RV64 contract before
implementation.

Actions:
- Identify the prepared fact shape that should authorize the failing route, or
  the missing fact that should keep it fail-closed.
- Add focused backend or prepared coverage for one legal shape matching the
  proven owner.
- Add at least one negative coverage case for missing, stale, ambiguous, or
  mismatched authority at the same boundary.
- Keep the GCC torture row as representative integration evidence, not the
  only proof surface.

Completion check:
- Focused tests fail for the known missing behavior or pass with a precise
  fail-closed diagnostic, and the expected producer/consumer fact names are
  documented in executor notes.

## Step 3: Repair The Proven Callee/Frame-Slot Rule

Goal: Implement the smallest general repair for the proven owner.

Actions:
- If the owner is call-result propagation, repair prepared publication or RV64
  consumption of the callee result without guessing from ABI registers or
  testcase shape.
- If the owner is frame-slot value preservation or the `*d` store path, repair
  the prepared value-home or RV64 local-memory store/load rule only when the
  destination and freshness facts are explicit.
- Preserve the completed `%t6` branch materialization path and existing
  fail-closed behavior for unrelated call, branch, and local-memory shapes.
- Keep implementation changes scoped to the proven boundary and its focused
  tests.

Completion check:
- The focused positive coverage passes and the negative coverage still fails
  closed with the expected diagnostic or status.

## Step 4: Representative Runtime And Backend Regression Proof

Goal: Prove the repaired route against the representative row and backend
regression subset.

Actions:
- Re-run BIR, prepared-BIR, ASM, object, disassembly, and qemu runtime for
  `src/20140828-1.c`.
- Confirm the one-case c4c RV64 runtime matches clang, or record the next
  precise downstream owner if the original boundary is complete.
- Run the supervisor-selected backend proof command and preserve
  `test_after.log`.

Completion check:
- Backend regression proof shows no new backend failures.
- Any remaining failure is classified as downstream or outside this source
  idea, with no regression in `%t6` pointer-source publication or branch
  authority.
