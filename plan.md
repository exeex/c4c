# RV64 Loop-2e Indirect Store Writeback Runtime Runbook

Status: Active
Source Idea: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md

## Purpose

Repair or precisely classify the downstream RV64 runtime owner in
`tests/c/external/gcc_torture/src/loop-2e.c` after the completed `%t23`
pointer-source publication and branch-authority chain from idea 653.

## Goal

Make the `loop-2e.c` one-case RV64 runtime match clang, or fail closed at the
proven indirect-store/postincrement writeback owner with a precise diagnostic.

## Core Rule

Do not special-case `loop-2e.c`, `%t23`, `q[39]`, or callee `f`. Repair only a
general indirect-store or postincrement writeback rule backed by explicit
semantic/prepared destination and updated-pointer facts.

## Read First

- `ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md`
- `build/agent_state/653_step4a_loop_t23_runtime_probe/summary.md`, if present
- Existing BIR, prepared-BIR, ASM, object, disassembly, and runtime evidence
  generated for `tests/c/external/gcc_torture/src/loop-2e.c`

## Current Targets

- Representative source:
  `tests/c/external/gcc_torture/src/loop-2e.c`
- Callee indirect-store and postincrement writeback lowering in the RV64
  backend path
- Prepared value homes and RV64 stores that decide whether the caller-provided
  pointee is updated

## Non-Goals

- Do not reopen `%t23` semantic producer publication, clobber safety, branch
  RHS authority, or fused pointer branch terminator admission unless fresh
  evidence proves a regression.
- Do not change runtime policy, unsupported markers, expectation files,
  allowlists, timeouts, or pass/fail accounting.
- Do not implement stack-destination fan-in authority work from ideas 647/655
  under this plan.
- Do not claim progress through helper renames, diagnostic-only relabeling, or
  classification-only edits when the same qemu abort remains.

## Working Model

Idea 653 already repaired the `%t23` pointer-source boundary: BIR and prepared
BIR publish `%t23 = bir.add ptr %t21, 156`, branch RHS authority is available
for the refreshed `%t23` home, and RV64 object emission exits 0. The remaining
runtime abort is downstream: callee `f` updates local `q` storage at `0(sp)`
instead of storing the computed pointer through the caller-provided `*q++`
destination, so `main` later reloads unchanged `q[39]`.

## Execution Rules

- Start each packet from refreshed evidence, not from stale assumptions.
- Keep evidence under `build/agent_state/657_*`.
- Use focused one-case probes before broader backend proof.
- Preserve completed `%t23` source publication and branch-authority behavior.
- If required semantic/prepared destination facts are missing, stop at the
  first missing producer boundary and record a precise fail-closed owner.
- For code-changing packets, run `cmake --build --preset default` plus the
  delegated focused proof before backend regression proof.

## Steps

### Step 1: Refresh Loop-2e Runtime Boundary Evidence

Goal: Re-establish the exact current `loop-2e.c` RV64 failure boundary.

Actions:

- Regenerate BIR, prepared-BIR, ASM, object, disassembly, and one-case runtime
  evidence for `tests/c/external/gcc_torture/src/loop-2e.c`.
- Compare c4c RV64 runtime with clang runtime for the same case.
- Confirm whether `%t23 = bir.add ptr %t21, 156`, branch RHS authority, and
  RV64 object emission still hold.
- Identify the first callee indirect-store or postincrement writeback point
  where the caller-visible update is lost.
- Record logs and a short summary under `build/agent_state/657_step1_loop_2e_runtime_boundary/`.

Completion Check:

- Evidence names the first wrong-value or fail-closed boundary without
  reopening the completed `%t23` producer/branch-authority route.

### Step 2: Trace Destination And Updated-Pointer Facts

Goal: Determine whether semantic and prepared layers explicitly describe the
caller-provided pointee destination and the postincremented pointer value.

Actions:

- Trace the callee `f` source pointer, indirect-store destination, and
  postincrement writeback through BIR and prepared value homes.
- Distinguish stores to the local pointer variable home from stores through
  the caller-provided pointee.
- Identify the exact prepared fact or missing fact that RV64 lowering should
  consume.
- Add focused dump coverage only if existing dump tests cannot prove the
  required producer and consumer facts.

Completion Check:

- The route has either a positive producer/consumer contract for the indirect
  store and writeback, or a precise missing-fact diagnostic owner that blocks
  RV64 object repair.

### Step 3: Repair General RV64 Indirect-Store Or Writeback Lowering

Goal: Make RV64 lowering consume the proven facts so the caller-visible
`*q++` update is emitted correctly.

Actions:

- Implement the smallest general lowering change that targets the proven
  indirect-store or postincrement writeback contract.
- Preserve fail-closed diagnostics for missing, stale, ambiguous, or mismatched
  destination/update facts.
- Add or update focused backend tests that prove the legal shape and at least
  one negative state.
- Do not infer authority from source filename, value id, stack slot existence,
  final assembly shape, runtime behavior, or diagnostic text.

Completion Check:

- The focused legal case either reaches matching RV64 runtime behavior or the
  object route fails closed at a newly precise, non-overfit owner.

### Step 4: Representative Runtime And Backend Regression Proof

Goal: Prove the accepted route on `loop-2e.c` and verify backend regression
state.

Actions:

- Refresh representative `loop-2e.c` runtime evidence after the accepted
  repair or fail-closed classification.
- Run the supervisor-delegated backend proof, normally:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Compare proof counts against the matching baseline selected by the
  supervisor.
- Record final evidence under `build/agent_state/657_step4_representative_proof/`.

Completion Check:

- `loop-2e.c` either matches clang under RV64 runtime or has a precise
  fail-closed downstream owner, and backend proof shows no new backend
  regressions.
