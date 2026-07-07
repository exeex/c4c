# BIR Vector Binop Semantic Producer Admission Runbook

Status: Active
Source Idea: ideas/open/585_bir_vector_binop_semantic_producer_admission.md

## Purpose

Activate the vector arithmetic binary-operation producer boundary split from
the closed scalar-binop lane and the separate wide-vector ABI signature owner
decision.

## Goal

Decide and repair, or explicitly fail-close, BIR vector arithmetic binary
operation producer admission after vector function signature facts are
available.

## Core Rule

Do not claim vector-binop producer progress unless supported vector binary
opcode and operand facts are published at the real instruction-lowering
boundary, or the boundary is explicitly fail-closed and covered.

## Read First

- `ideas/open/585_bir_vector_binop_semantic_producer_admission.md`
- `ideas/closed/562_bir_scalar_binop_semantic_producer_admission.md`
- `ideas/closed/563_bir_wide_vector_abi_signature_representation_owner_decision.md`
- BIR LIR-to-BIR instruction-lowering code that publishes arithmetic binary
  opcode and operand facts.
- Focused BIR tests for arithmetic binary operations, fixed-vector operands,
  unsupported/fail-closed diagnostics, and representative vector torture rows
  such as `src/simd-6.c` or a stronger current substitute.

## Current Scope

- Actual BIR producer admission boundary for vector arithmetic binary
  operations after function signature facts exist.
- Focused BIR coverage for supported vector binary opcode and operand fact
  publication, or for an explicit fail-closed vector-binop owner boundary.
- Representative proof that `src/simd-6.c`, or a stronger current substitute,
  advances beyond the old vector-binop semantic admission diagnostic only when
  BIR facts are correct.
- Downstream-owner recording for newly exposed scalar-cast, local-memory, ABI,
  or RV64 lowering failures.

## Non-Goals

- Scalar-binop F128 arithmetic repair; that belongs to the closed parent idea.
- Wide-vector function signature ABI carrier decisions; those belong to
  `ideas/closed/563_bir_wide_vector_abi_signature_representation_owner_decision.md`.
- Scalar-cast, scalar/local-memory, alloca local-memory, RV64 object lowering,
  expectation rewrites, unsupported downgrades, allowlist changes, or
  classification-only movement.
- Broad vector or ABI rewrites that leave the same vector-binop producer
  ambiguity in place.

## Working Model

Small-vector signature admission can expose vector arithmetic instructions to
the BIR semantic producer. This runbook owns the vector-binop admission
decision at the instruction-lowering boundary. It does not own scalar-binop
repair, wide-vector ABI signature representation, or downstream lowering
families that appear only after correct vector-binop facts are available.

## Execution Rules

- Keep routine evidence, representative diagnostics, and proof output in
  `todo.md`.
- Start by refreshing the actual current vector-binop failure boundary before
  choosing an implementation route.
- Add focused BIR coverage before claiming vector-binop producer progress.
- If the correct decision is to keep vector binops fail-closed, make that owner
  boundary explicit and covered instead of weakening expectations.
- Reject named-case handling for only `src/simd-6.c` or one vector spelling.
  Prefer semantic opcode and operand rules.
- Do not rewrite expectations, unsupported markers, allowlists, row
  classifications, or outer failure-note text as evidence of producer progress.
- If a representative advances to scalar-cast, local-memory, ABI, RV64, or
  another downstream owner boundary, record that boundary in `todo.md`; do not
  expand this plan into downstream work.
- For code-changing steps, run the supervisor-delegated proof command. If no
  narrower command is delegated, use:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Step 1: Refresh Vector-Binop Producer Evidence

Goal: Determine the current vector-binop BIR semantic producer boundary and
whether existing coverage already documents part of the intended owner
decision.

Actions:
- Inspect focused BIR arithmetic binary-operation tests for vector opcode and
  operand coverage or fail-closed diagnostics.
- Inspect the real instruction-lowering path that admits arithmetic binary
  operations into BIR facts.
- Re-run direct diagnostics for `src/simd-6.c`, or a stronger current
  vector-binop substitute if the inventory has moved.
- Identify whether the representative fails at vector-binop admission,
  signature admission, scalar-cast, local-memory, ABI, RV64 lowering, or
  another owner family.
- Distinguish vector-binop producer ambiguity from wide-vector ABI signature
  carrier gaps and downstream lowering failures.
- Run the delegated backend proof command and refresh `test_after.log`.
- Record the representative set, current owner boundary, and next packet
  recommendation in `todo.md`.

Completion Check:
- `todo.md` names the refreshed representative set and current vector-binop or
  downstream owner boundary.
- `test_after.log` contains a fresh proof result for the delegated command.
- The next packet is either Step 2 for real vector-binop producer repair or
  Step 3 for broader validation and closure handoff.

## Step 2: Repair Or Fail-Close Vector-Binop Producer Admission

Goal: Publish correct vector binary opcode and operand facts, or define the
explicit fail-closed vector-binop owner boundary.

Actions:
- Work only in the real BIR instruction-lowering path for arithmetic binary
  operations and its focused tests.
- Define the supported vector opcode and operand fact rule before changing
  admission behavior.
- Add or tighten focused BIR coverage for the selected supported vector-binop
  behavior or fail-closed diagnostic.
- Avoid testcase-shaped matching, vector-spelling shortcuts, or scalar-binop
  helper reuse that hides the same vector-binop ambiguity behind a new name.
- Prove the focused tests and the supervisor-delegated backend subset.
- Update `todo.md` with the selected producer rule, proof command, and any
  downstream owner boundary discovered.

Completion Check:
- Vector arithmetic binary operations are either admitted with correct
  test-covered BIR opcode and operand facts or rejected at a documented
  fail-closed owner boundary.
- Representative movement, if any, comes from correct BIR vector-binop facts
  and not expectation, unsupported, allowlist, or classification changes.
- Backend proof is green.

## Step 3: Broader Validation And Closure Handoff

Goal: Decide whether the source idea is complete after refreshed evidence and
any needed vector-binop producer work.

Actions:
- Run the supervisor-delegated broader validation command, or at minimum the
  backend subset if no broader command is delegated.
- Confirm that remaining failures, if any, are downstream owner boundaries and
  not ambiguous vector-binop producer failures.
- Record closure evidence and residual risks in `todo.md`.
- Ask the plan owner to decide whether to close, continue, or split a new open
  idea.

Completion Check:
- `todo.md` contains enough current proof for a close decision.
- The source idea is either ready for close gate review or has a concrete
  remaining vector-binop producer packet.
