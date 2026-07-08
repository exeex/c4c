# Pointer/Address Semantic Model Research

Status: Open
Type: Research and architecture documentation
Parent: `ideas/open/595_prepared_value_architecture_followup_umbrella.md`
Related:
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
- `ideas/open/591_prepared_mir_view_contract_research.md`
Owning Layer: pointer/address semantic model

## Goal

Define the pointer/address semantic model that prepared value freshness,
local-memory boundary checks, relocation materialization, and later MIR views
must consume. The output should decide which pointer/address facts are
semantic authority, which are support/proof facts, which belong to target
consumption, and which are diagnostic-only.

This is a research route, not an implementation route. It should not migrate
target consumers or reopen closed branch stack-source work.

## Why This Exists

The recent freshness chain closed several concrete prepared-value problems,
including representative source freshness, direct edge-publication source
ownership, branch stack-load freshness, and RV64 pointer branch stack-source
consumption. Those closures deliberately solve narrow use-specific freshness
questions.

They do not define the general pointer/address semantic model. Older evidence
still shows that pointer arithmetic materialization, local-memory layout,
stack-home completeness, relocation meaning, and target operand shape can be
confused with semantic pointer freshness. Idea 591 can later expose
MIR-facing pointer/address facts, but it should not invent the semantic model
inside the view boundary.

## Prerequisites

- Use the closed freshness authority chain from ideas 587 through 590 as the
  selected-authority baseline.
- Treat closed ideas 592, 593, 594, and 596 as narrow branch pointer
  stack-source evidence, not global pointer/address closure.
- Keep idea 591 as a downstream consumer of this model, not as the owner of
  unresolved pointer/address semantics.

## In Scope

- Inventory prepared pointer/address families that currently rely on
  pointer arithmetic materialization, semantic relocation, local stack or
  frame-slot addressing, local-array address derivation, global address
  materialization, branch pointer operands, or target-local operand shape.
- Define the semantic authority for each surveyed family, including when a
  pointer value is current for a use and when address calculation is only a
  support fact.
- Classify facts as required semantic authority, verifier/support fact,
  target consume fact, route proof, or diagnostic-only artifact.
- State fail-closed rules for missing, ambiguous, stale, wrong-value,
  wrong-use, stack-home-only, local-layout-only, relocation-less, or
  target-shape-only evidence.
- Recommend one to three later implementation ideas only when each proposed
  idea has a single first owner, prerequisites, proof surface, and reject
  signals.

## Out Of Scope

- Implementation changes to prepared/prealloc, MIR, targets, tests,
  expectations, unsupported markers, allowlists, runtime behavior, or harness
  behavior.
- Reopening the closed RV64 fused pointer branch stack-slot `Lhs` and `Rhs`
  publication or consumption queue.
- Folding target RV64/AArch64/x86 consumer migration, shared producer
  publication repair, and MIR view migration into one implementation route.
- Treating local-memory layout, stack-home completeness, target operand shape,
  or diagnostic dumps as semantic pointer/address authority.

## Acceptance Criteria

- The research documents identify the surveyed pointer/address families and
  cite the concrete prepared, prealloc, MIR, or target surfaces that consume
  them.
- The output states a semantic authority rule for each surveyed family and a
  fail-closed rule for missing or invalid evidence.
- The closed branch pointer stack-source queue is explicitly classified as a
  narrow selected-freshness subset, not as global pointer/address semantic
  closure.
- The output separates semantic authority from support/proof facts,
  target-consume facts, and diagnostic-only artifacts.
- Any recommended implementation follow-up is split by first owning layer and
  includes prerequisites, proof surface, and reviewer reject signals.
- No implementation files, tests, expectations, unsupported markers,
  allowlists, runtime behavior, active plan state, or lifecycle history are
  changed.

## Reviewer Reject Signals

- Reject testcase-shaped analysis that proves the model through one named
  source file, one branch shape, one target operand shape, or one local-memory
  fixture while ignoring nearby pointer/address families.
- Reject expectation downgrades, unsupported-marker edits, allowlist changes,
  runtime-output changes, or diagnostic text changes as pointer/address
  semantic progress.
- Reject treating stack-home completeness, local-memory layout, target
  operand shape, or target-local inference as pointer freshness.
- Reject hiding unresolved pointer/address semantics behind
  `PreparedMirView`, a renamed helper, or a new abstraction with the same
  stale-home or stack-home-only failure mode.
- Reject broad mixed-owner implementation proposals that combine semantic
  model research, shared-prealloc producer publication, target consumer
  migration, and Prepared MIR view design.

## Closure Note Requirements

The closure note must state:

1. Which pointer/address families were surveyed.
2. Which facts are semantic authority for each family.
3. Which facts are verifier/support facts, target consume facts, route proofs,
   or diagnostic-only artifacts.
4. Which existing closed ideas are evidence and which gaps they deliberately
   do not close.
5. Which concrete later implementation ideas, if any, should be opened.
6. Which families remain deferred because the first owner or proof surface is
   still unclear.
7. Whether idea 591 needs to consume, cite, or wait for any result from this
   research.
