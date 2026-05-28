# AArch64 Dispatch Prepared Publication Seam Runbook

Status: Active
Source Idea: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Supersedes active runbook for: ideas/open/58_aarch64_prepared_authority_regression_recovery.md

## Purpose

Keep idea 60 in decomposition mode after the GOT `LoadGlobal` packet proved
useful integration progress, but before the remaining dirty AArch64 stack is
accepted as one bundle.

## Goal

Split the remaining dirty implementation stack into independently provable
seam packets. Each packet must either extract/prove one focused seam or leave
the related dirty code explicitly unaccepted.

## Core Rule

Do not commit the remaining dirty implementation stack as one coherent slice.
`review/remaining_dirty_stack_acceptance_review.md` classifies it as a
multi-seam bundle: useful context, not acceptance-ready progress.

## Read First

- `ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md`
- `ideas/open/58_aarch64_prepared_authority_regression_recovery.md`
- `review/remaining_dirty_stack_acceptance_review.md`
- `review/step3_dispatch_route_review.md`
- `todo.md`

## Current Targets

- Current integration shape reported by the supervisor: the focused four-test
  proof is `3/4`; only `c_testsuite_aarch64_backend_src_00196_c` still fails.
- Accepted checkpoint: the committed GOT-required prepared `LoadGlobal`
  materialization packet.
- Remaining dirty seams that still need independent ownership and proof:
  store-local selected publication, store-global stack publication,
  fused-compare selected operand order, call/outgoing stack argument
  materialization, and direct edge `LoadLocal` prepared source-memory
  consumption.

## Non-Goals

- Do not close idea 60 or idea 58 from this runbook rewrite.
- Do not treat the `3/4` integration result as acceptance proof for the
  remaining dirty stack.
- Do not weaken any dispatch, backend route, or c-testsuite contract.
- Do not add testcase-name checks, exact temporary-name checks, literal label
  checks, or fixed stack-offset checks.
- Do not touch unrelated AArch64 seams while proving the selected seam.

## Working Model

The dirty implementation stack is not hard-rejected as testcase-overfit, but it
spans several idea 60 seams. The next route must preserve the useful evidence
without collapsing back into a monolithic
`backend_aarch64_instruction_dispatch` assertion chase.

`c_testsuite_aarch64_backend_src_00196_c` remains owned by the idea 58
`78730af2f` family unless a focused idea 60 seam proves shared ownership.

## Execution Rules

- Use one focused probe and one backend owner surface per implementation
  packet.
- Commit only a seam whose focused proof is fresh and whose integration result
  is recorded in `todo.md`.
- Keep unproven dirty code unaccepted; do not bundle it with a proven seam.
- If a focused probe cannot be expressed in the backend case harness, record
  the harness gap in `todo.md` and stop before accepting the dirty code.
- Run build proof before any code-changing packet is considered
  acceptance-ready.
- Run the focused four-test proof as integration confirmation; `00196` may
  remain red unless the selected seam claims to own it.

## Steps

### Step 1: Establish The Blocked Failure-Family Baseline

Status: Complete.

Completion check:

- `todo.md` recorded the blocked baseline, dirty context, current first bad
  assertion, and decomposition as the active route.

### Step 2: Inventory The Separable Dispatch Seams

Status: Complete.

Completion check:

- `todo.md` mapped visible seams to likely owner surfaces and evidence sources.

### Step 3: Split The Monolithic Probe Into Focused Cases

Status: Complete.

Completion check:

- `todo.md` listed selected or proposed focused probes for the visible seams.

### Step 4: Bind The GOT LoadGlobal Implementation Route

Status: Complete.

Completion check:

- `todo.md` selected the GOT-required prepared `LoadGlobal` probe, owner
  surface, proof command, and do-not-touch boundaries.

### Step 5: Prove GOT-Required Prepared LoadGlobal Materialization

Status: Complete and committed.

Completion check:

- The GOT-focused route probe passed.
- `backend_aarch64_instruction_dispatch` no longer reports the
  GOT-required `LoadGlobal` materialization assertion as first bad.

### Step 6: Prove Store-Local Selected Publication Ownership

Status: Blocked at C-route probe; continue only through a lower-level
MIR/unit ownership probe.

Goal: decide whether the dirty store-local future-consumer suppression is a
valid prepared-publication ownership rule.

Primary target: `memory.cpp` / `dispatch.cpp` store-local selected publication
accounting, with proof moved below the C backend route harness because the
candidate C probes did not create the required stack-homed selected producer.

Actions:

- Start from the dirty store-local future-consumer suppression called out in
  `review/remaining_dirty_stack_acceptance_review.md`.
- Treat the attempted C route for
  `backend_codegen_route_aarch64_store_local_selected_publication` as
  non-acceptance evidence: it showed that route-level local-store compilation
  can avoid the intended stack-homed selected-producer shape.
- Next executor packet must add or extend a lower-level MIR/unit probe for
  `future_store_local_stack_value_publication_covers_instruction`, preferably
  in `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  unless a closer existing MIR test already owns prepared store-source
  publication accounting.
- The probe must construct prepared BIR where the current instruction produces
  a named, stack-homed selected value and a later same-block `StoreLocal`
  publication plan covers that exact producer or its direct global select-chain
  root.
- The same probe must also fail closed when the future store is absent, uses a
  different value/type, has no available prepared store-source publication, or
  covers a register-homed source instead of a stack home.
- If implementation is delegated after the probe exists, keep code edits
  limited to `future_store_local_stack_value_publication_covers_instruction`
  and its direct store-local ownership helper surface. Do not broaden the
  helper into a generic future-consumer scan.
- Do not modify calls, store-global publication, fused-compare materialization,
  direct edge `LoadLocal`, or GOT `LoadGlobal` code in this packet.
- If that lower-level probe cannot be built without exposing inappropriate
  private internals or only repeating the broad dispatch case, retire the dirty
  suppression from the acceptance path: the next executor should remove or
  quarantine the store-local dirty code without accepting it, while preserving
  the remaining dirty-stack warning for the other seams.

Completion check:

- A focused MIR/unit store-local ownership probe passes with build proof and
  proves both the positive and fail-closed cases, or `todo.md` records that the
  dirty suppression was removed/quarantined and remains unaccepted.
- The focused four-test proof is rerun as integration confirmation, with any
  remaining `00196` failure kept separate unless this seam proves ownership.

### Step 7: Prove Store-Global Stack Publication Ownership

Goal: prove or leave unaccepted the dirty store-global stack publication and
republishing behavior.

Primary target: `memory.cpp` store-global prepared publication accounting.

Completion check:

- A focused store-global probe passes, or `todo.md` records a harness gap and
  the dirty store-global code remains unaccepted.

### Step 8: Prove Fused-Compare Selected Operand Order

Goal: prove or leave unaccepted the dirty fused-compare scratch preference for
select-materialized operands.

Primary target: `dispatch_publication.cpp` fused-compare operand
materialization.

Completion check:

- A focused fused-compare selected-operand-order probe passes without binding
  to temporary register names, or the dirty fused-compare code remains
  unaccepted.

### Step 9: Prove Call And Outgoing Stack Argument Materialization

Goal: prove or leave unaccepted the dirty call/outgoing stack argument
materialization fallbacks.

Primary target: `calls.cpp` prepared call-plan and move-bundle lookup paths.

Completion check:

- A focused call stack-argument probe passes, or `todo.md` records why the
  dirty call code remains unaccepted.

### Step 10: Prove Direct Edge LoadLocal Prepared Memory Consumption

Goal: prove or leave unaccepted the dirty direct edge `LoadLocal` prepared
source-memory fail-closed behavior.

Primary target: `dispatch_edge_copies.cpp` edge `LoadLocal` publication.

Completion check:

- A focused edge-load-local prepared-memory probe passes, or `todo.md` records
  the harness gap and keeps the dirty edge code unaccepted.

### Step 11: Reconnect To Idea 58 Recovery Criteria

Goal: decide whether idea 60 decomposition has given idea 58 a narrow enough
route to resume or close.

Actions:

- Rerun the supervisor-selected four-test proof after the accepted seam
  packets.
- Confirm whether only the idea 58 `00196` family remains.
- Ask the supervisor to route closure, continuation, or switch decisions
  through the lifecycle after the focused seams are independently owned.

Completion check:

- The supervisor can either resume idea 58 with a narrow remaining route or
  close idea 60 because each dirty seam has focused ownership/proof or an
  explicit unaccepted status.
