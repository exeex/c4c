# BIR Wide Vector ABI Signature Representation Runbook

Status: Active
Source Idea: ideas/open/563_bir_wide_vector_abi_signature_representation_owner_decision.md

## Purpose

Activate the wide-vector ABI signature carrier owner decision split from the
function-signature producer lane.

## Goal

Define, implement, or explicitly fail-close the BIR ABI representation contract
for 16-byte and 32-byte LLVM vector function signatures before publishing
return-info or parameter-layout metadata.

## Core Rule

Do not admit wide LLVM vector signatures through scalar, VRM, split-register, or
memory-like metadata until the BIR ABI carrier contract for that representation
is explicit and test-covered.

## Read First

- `ideas/open/563_bir_wide_vector_abi_signature_representation_owner_decision.md`
- `src/backend/bir/lir_to_bir/call_abi.cpp`
- Existing BIR tests that cover return ABI metadata, parameter ABI metadata,
  fixed-vector signatures, and fail-closed signature diagnostics.
- Representative torture rows such as `src/ieee/pr72824-2.c` / `foo` and
  `src/pr70903.c` / `foo`, or stronger current substitutes if the inventory has
  moved.

## Current Scope

- BIR ABI carrier representation for 16-byte and 32-byte LLVM vector return and
  parameter signatures.
- Return-info and parameter-layout publication in the BIR LIR-to-BIR call ABI
  path.
- Focused BIR coverage that proves either admitted wide-vector signature facts
  or a deliberate fail-closed owner boundary.
- Downstream-owner recording for failures exposed after correct signature facts
  are published.

## Non-Goals

- Scalar-binop, vector-binop, scalar-cast, local-memory, alloca, or RV64 object
  emission repair after function signature facts are correct.
- Expectation rewrites, unsupported downgrades, allowlist changes, or
  classification-only movement.
- Named-case handling for one torture row or one vector spelling without a
  general ABI representation rule.
- Broad call/ABI rewrites that retain the same wide-vector signature carrier
  ambiguity under a new helper name.

## Working Model

Small fixed-vector signatures are already admitted by the earlier
function-signature producer work. Wider LLVM vector signatures remain
fail-closed because the current metadata model has not chosen whether those
values are represented as scalar carriers, multiple lanes, split registers,
memory, or another explicit ABI form. This runbook owns that representation
decision, then applies it only at the return-info and parameter-layout
publication boundary.

## Execution Rules

- Keep routine evidence, representative diagnostics, and proof output in
  `todo.md`.
- Start by refreshing the actual current failure boundary before choosing an
  implementation route.
- Add focused BIR coverage before claiming wide-vector ABI signature progress.
- If the correct decision is to keep wide vectors fail-closed, make that owner
  boundary explicit and covered instead of weakening expectations.
- If a representative advances to vector-binop, scalar-cast, local-memory,
  alloca, RV64, or another downstream owner boundary, record that boundary in
  `todo.md`; do not expand this plan into that downstream work.
- For code-changing steps, run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Step 1: Refresh Wide-Vector ABI Signature Evidence

Goal: Determine the current wide-vector signature boundary and whether the
existing code already documents part of the intended owner decision.

Actions:
- Inspect focused BIR ABI tests for wide fixed-vector return and parameter
  coverage or fail-closed diagnostics.
- Re-run direct diagnostics for `src/ieee/pr72824-2.c` / `foo`,
  `src/pr70903.c` / `foo`, or stronger current wide-vector signature
  substitutes.
- Identify whether each representative fails at return-info publication,
  parameter-layout publication, a later vector-binop boundary, or another owner
  family.
- Distinguish ABI carrier ambiguity from downstream arithmetic or object
  lowering failures.
- Run the delegated backend proof command and refresh `test_after.log`.
- Record the representative set, current owner boundary, and next packet
  recommendation in `todo.md`.

Completion Check:
- `todo.md` names the refreshed representative set and current wide-vector ABI
  signature or downstream owner boundary.
- `test_after.log` contains a fresh backend proof result.
- The next packet is either Step 2 for a real representation decision and
  producer repair, or Step 3 for broader validation and closure handoff.

## Step 2: Decide And Implement Wide-Vector ABI Carrier Contract

Goal: Make the 16-byte and 32-byte LLVM vector ABI signature representation
explicit, then repair only the matching publication path.

Actions:
- Work in the real return-info and parameter-layout publication path in
  `call_abi.cpp`.
- Define the supported carrier shape, or define the fail-closed owner boundary,
  for wide LLVM vectors before changing admission behavior.
- Add or tighten focused BIR coverage for the selected carrier contract or
  fail-closed diagnostic.
- Avoid mapping wide vectors to `I128`, scalar integer pairs, VRM placeholders,
  or ad hoc multi-register metadata unless that is the explicit documented BIR
  ABI representation.
- Prove the focused tests and the backend subset.
- Update `todo.md` with the selected representation rule, proof command, and
  any downstream owner boundary discovered.

Completion Check:
- Wide-vector return and parameter signatures are either admitted with correct
  test-covered ABI metadata or rejected at a documented fail-closed owner
  boundary.
- Representative movement, if any, comes from correct BIR signature facts and
  not expectation, unsupported, allowlist, or classification changes.
- Backend proof is green.

## Step 3: Broader Validation And Closure Handoff

Goal: Decide whether the source idea is complete after refreshed evidence and
any needed representation work.

Actions:
- Run the supervisor-delegated broader validation command, or at minimum the
  backend subset if no broader command is delegated.
- Confirm that remaining failures, if any, are downstream owner boundaries and
  not ambiguous wide-vector signature carrier failures.
- Record closure evidence and residual risks in `todo.md`.
- Ask the plan owner to decide whether to close, continue, or split a new open
  idea.

Completion Check:
- `todo.md` contains enough current proof for a close decision.
- The source idea is either ready for close gate review or has a concrete
  remaining wide-vector ABI signature packet.
