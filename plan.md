# RV64 Object Route RHS Bitfield Boolean Select Abort Runbook

Status: Active
Source Idea: ideas/open/411_rv64_object_route_rhs_bitfield_boolean_select_abort.md
Activated after: ideas/closed/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md

## Purpose

Repair the remaining `20000113-1.c` runtime abort after 402 separated it from
the fixed frame-slot local-address segfault family.

## Goal

Classify and repair the first supportable RV64 object value
scheduling/control-flow family around RHS bitfield/boolean materialization and
select/join handling so the representative matches clang under qemu or
advances to a precisely routed residual.

## Core Rule

This is runtime correctness work. Do not claim compile-only progress, weaken
qemu comparison, or patch around the abort with testcase-shaped control-flow.

## Read First

- `ideas/open/411_rv64_object_route_rhs_bitfield_boolean_select_abort.md`
- `ideas/closed/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md`
- `ideas/closed/381_rv64_object_route_short_circuit_select_join_materialization.md`
- `tests/c/external/gcc_torture/src/20000113-1.c`
- Current proof artifacts under
  `build/agent_state/402_step3_runtime_residual_proof/20000113-1/`

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/20000113-1.c`
- Function area: `foobar`
- Prepared values and join path: `%t43/%t44/%t48`,
  `join_transfer logic.end.40 result=%t48 kind=phi_edge
  carrier=select_materialization ownership=authoritative_branch_pair`

## Non-Goals

- Do not reopen 402's frame-slot local-address repair for `20070212-2.c`.
- Do not reopen idea 381 unless fresh evidence proves that exact closed rule
  regressed.
- Do not broaden into unrelated runtime mismatch families.
- Do not use compile-only proof for this qemu-reaching case.
- Do not use filename-specific branches, expectation rewrites, qemu weakening,
  unsupported downgrades, or allowlist filtering.

## Working Model

The current `20000113-1.c` proof reports `clang_qemu_status=0` but
`c4c_qemu_status=134` and `c4c_qemu_strace_status=134`. The object reaches
`abort()` after stale or mis-scheduled value handling around RHS
bitfield/boolean materialization and select/join handling. Step 1 should
classify whether the first owner is RV64 object scheduling, control-flow
placement, select/join materialization, or a prepared producer fact gap.

## Execution Rules

- Start with classification proof before implementation.
- Compare clang and c4c qemu behavior for the representative.
- Inspect prepared BIR, emitted object disassembly, and qemu/strace evidence
  around `foobar` before editing backend code.
- Preserve branch, select/join, value-home, bitfield, boolean, and runtime
  semantics.
- If prepared select/join or value facts are wrong or missing, route that
  producer boundary instead of compensating in RV64 object emission.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat return-code-only wins, diagnostic-only churn, expectation rewrites,
  allowlist filtering, and named-case green proof as route failures.

## Step 1: Classify The RHS Bitfield Boolean Abort

Goal: identify the exact value scheduling/control-flow or producer-fact owner
for the `20000113-1.c` abort.

Actions:

- Reproduce or inspect the fresh clang-vs-c4c qemu proof for `20000113-1.c`.
- Record qemu exits, strace abort evidence, prepared select/join facts, value
  homes, and emitted object code around `foobar`.
- Trace `%t43/%t44/%t48` from prepared facts through emitted object code and
  the branch path that reaches `abort()`.
- Decide whether the first repair packet is object value scheduling, branch
  placement, select/join materialization, or producer fact routing.

Completion check:

- `todo.md` records the concrete runtime family for the first executor packet,
  the representative set, and the exact supervisor-delegated proof command.
- Any non-411 residual is routed with precise evidence.

## Step 2: Repair The First Supported Runtime Owner

Goal: repair the first classified value scheduling/control-flow family while
preserving qemu-observable behavior.

Actions:

- Update the correct RV64 object-route or producer layer for the selected root
  cause.
- Preserve bitfield/boolean value semantics, select/join ownership,
  branch/control-flow placement, and runtime behavior.
- Add or update focused backend coverage for the repaired family and adjacent
  unsupported shapes.

Completion check:

- `20000113-1.c` no longer reaches the same runtime abort under qemu, or
  advances to a precisely routed residual.
- Existing backend coverage for adjacent local-address and select/join
  behavior remains green.

## Step 3: Prove Representative And Residual Ownership

Goal: prove the repair advanced 411 without hiding runtime correctness
failures.

Actions:

- Run the supervisor-selected clang-vs-c4c qemu proof for `20000113-1.c`.
- Inspect qemu comparison, object disassembly, and prepared facts.
- Run the supervisor-selected backend CTest subset.
- Classify any remaining failure as the same family, a distinct runtime
  family, a target-emission family, or a producer-fact gap.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, qemu weakening, allowlist filtering, masked
  producer defects, or filename-specific fixes are used as acceptance
  evidence.
- The supervisor has enough evidence to continue, request route review, or ask
  the plan owner for close/deactivation handling.
