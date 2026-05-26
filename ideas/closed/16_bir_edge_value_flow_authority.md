# BIR Edge Value Flow Authority

## Goal

Move CFG edge value-flow authority out of AArch64 dispatch and into shared BIR
semantics plus prepared edge-publication facts that future x86 and RISC-V
lowering can consume.

## Why This Exists

AArch64 `dispatch_edge_copies.*` currently does more than emit target moves. It
also rediscovers predecessor producers, join/select sources, block-entry
publication needs, and some parallel-copy safety decisions while lowering a
block. Those are semantic control-flow facts, not AArch64-specific facts.

This makes the AArch64 backend larger than it should be and creates a poor
starting point for x86 and RISC-V: each target would otherwise need to recreate
the same edge value-flow reasoning before it can emit target instructions.

The desired boundary is:

- BIR owns block-entry and edge value-flow semantics.
- Shared prepare turns those BIR facts plus value homes into concrete prepared
  edge-publication plans.
- AArch64, x86, and RISC-V consume those plans and only perform target-specific
  instruction selection.

## In Scope

- Audit AArch64 `dispatch_edge_copies.*`, `dispatch_publication.*`, and the
  dispatch call sites that infer predecessor/successor value flow.
- Identify which facts belong in BIR as edge/block-entry value semantics.
- Add shared prepared records or queries for edge-publication plans, including
  source value, destination home, phase, and predecessor/successor identity.
- Move reusable parallel-copy and redundant-copy decisions behind shared
  prepared helpers when they do not depend on target instruction encoding.
- Update AArch64 lowering to consume prepared edge-publication facts instead of
  rediscovering semantic edge producers locally.
- Preserve target-local emission for register names, scratch selection,
  addressing modes, immediate encodability, and actual instruction spelling.
- Add focused AArch64 tests that prove behavior is unchanged while the semantic
  authority moves.

## Out Of Scope

- Implementing full x86 or RISC-V codegen.
- Moving target-specific instruction emission into BIR.
- Rewriting call ABI lowering.
- Replacing the whole AArch64 dispatch pipeline in one step.
- Treating file-count or line-count reduction as success without an authority
  move.

## Acceptance Criteria

- BIR or shared prepare is the source of truth for edge/block-entry value-flow
  facts needed by backend lowering.
- AArch64 no longer performs broad semantic producer rediscovery for edge-copy
  lowering when a prepared edge-publication fact should exist.
- AArch64 `dispatch_edge_copies.*` is reduced toward target emission and
  target-specific hazard handling.
- The prepared representation is target-neutral enough for future x86 and
  RISC-V lowering to consume.
- Existing AArch64 join, select, branch, and block-entry publication behavior
  remains covered by focused tests and backend validation.

## Closure Note

Closed after Step 5 boundary validation. Shared prepared edge-publication facts
and helpers now carry the authority needed for covered edge/block-entry
publication paths, while AArch64 consumes those prepared facts and keeps
target-local emission, diagnostics, scratch, and hazard handling. Reviewer
report `review/edge_authority_step5_boundary_review.md` found no blocking
drift, testcase overfit, or expectation weakening; the remaining AArch64
producer lookup is not the authority path for prepared edge publications.

Close proof used `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R "^backend_"` in `test_after.log`, passing 162/162, plus
the regression guard against `test_before.log`.

## Reviewer Reject Signals

- A patch only renames `dispatch_edge_copies.*` without moving authority.
- A patch moves AArch64 register names, scratch-register policy, or assembly
  spelling into BIR.
- A patch creates x86/RISC-V-specific assumptions in shared prepared records.
- A patch adds testcase-shaped producer lookup instead of expressing general
  edge value-flow facts.
- A patch weakens existing AArch64 edge/publication expectations to make the
  migration appear green.
