# Pointer/Address Semantic Model Research

Status: Closed
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

## Closure Note

Closed after completion of the research package under
`docs/pointer_address_semantic_model_research/`.

Surveyed families:

- Pointer base plus offset value homes.
- BIR pointer arithmetic materialization for frame addresses.
- Semantic relocation and global address materialization.
- Global symbol memory accesses.
- Local stack or frame-slot addressing.
- Pointer-value indirect memory accesses.
- Local-array source object and address derivation.
- Local-array semantic GEP availability.
- Global static semantic GEP.
- Branch pointer stack-source operands.
- Target-local operand shape.

Semantic authorities and fact roles:

- Pointer base plus offset value homes remain deferred. The existing home
  shape, base name, symbol name, byte delta, stack/register placement, and
  target offset encodability are support or target-consume facts only until a
  selected pointer-arithmetic authority names base freshness, result identity,
  delta, use, and program point.
- BIR pointer arithmetic materialization for frame addresses is verifier and
  target-consume evidence for frame-address materialization, not pointer
  freshness authority.
- Semantic relocation and global address materialization are target-consume
  facts plus symbol-policy support for symbol, TLS, frame, or fixup
  materialization. Relocation records are not pointer freshness authority.
- Global symbol memory accesses have semantic address/range authority for the
  exact symbol-backed memory access when prepared global-symbol publication
  authority accepts the same symbol, layout, extent, range, and use. They do
  not authorize loaded-value freshness, store-source freshness, pointer-value
  freshness, or unrelated GEP validity.
- Local stack or frame-slot addressing is verifier/support and target-consume
  evidence for exact local memory operands. Value freshness still requires a
  separate use-specific selected authority.
- Pointer-value indirect memory accesses remain deferred. Current pointer
  value memory range/layout proof supports address legality and target memory
  operands, but it does not prove that the named pointer value is fresh at the
  exact load/store use.
- Local-array source objects, derivation records, element paths, checker
  inputs, range proofs, and local-address provenance are route proof and
  verifier/support facts. They become semantic address-derivation authority
  only through semantic GEP availability.
- Local-array semantic GEP `Available` records are semantic authority for the
  selected local-array address derivation. Non-`Available` statuses must stay
  rejected, and target consumers still need appropriate consumption facts.
- Global static semantic GEP `Available` records are semantic authority for
  the selected global static address derivation. Relocation, range, or target
  operand shape alone cannot replace that authority.
- Branch pointer stack-source operands have semantic freshness authority only
  for the closed RV64 fused pointer branch stack-slot `Lhs`/`Rhs` use when the
  selected `BranchStackLoadSource` / `BranchStackSlot` fact matches the exact
  value/home, proof, rank, branch block, and terminator point.
- Target-local operand shape, printed assembly, registers, immediates,
  memory operands, fixups, diagnostics, dumps, and candidate counts are
  target-consume or diagnostic-only artifacts. They never authorize semantic
  pointer/address validity by themselves.

Fail-closed rules:

- Missing, ambiguous, stale, wrong-value, wrong-use, stack-home-only,
  local-layout-only, relocation-less, relocation-only, range-only,
  target-shape-only, and diagnostic-only evidence must reject semantic
  pointer/address use.
- Support facts may explain, prove, or feed an already-authorized route, but
  they must not promote themselves into semantic authority.
- Later implementation proof should vary authority dimensions such as value
  identity, use kind, program point, derivation coordinate, source object or
  symbol, range authority, and fact class instead of proving one named
  testcase or one target operand shape.

Closed evidence boundaries:

- Ideas 587 through 590 are the selected freshness authority baseline. They
  establish that selected freshness is use-specific and must match value,
  use, source, proof, rank, source reference, and program point before a
  freshness claim is accepted. They do not define a global pointer/address
  model.
- Ideas 592, 593, 594, and 596 are narrow branch pointer stack-source
  evidence. They prove producer publication and RV64 consumption for the
  exact fused pointer branch stack-slot `Lhs`/`Rhs` subset. They do not close
  pointer arithmetic, pointer-value indirect memory, local-array/global GEP
  target consumption, relocation semantics, local-memory layout semantics,
  stack-home completeness, aggregate-adjacent branches, select/call/
  publication consumers, or target operand-shape semantics.

Generated follow-up ideas:

- `ideas/open/599_pointer_base_plus_offset_selected_authority.md` should
  define selected authority for pointer base plus offset uses and prove that
  stale base, wrong result, wrong delta, wrong use, range-only, and
  target-shape-only evidence fails closed.
- `ideas/open/600_pointer_value_memory_use_freshness_authority.md` should
  define selected freshness for pointer-value indirect memory uses and prove
  that range/layout/target-shape evidence remains support, not pointer
  freshness.

Deferred families:

- Local-array and global static semantic GEP target consumption is deferred
  until idea 591 or a later target-consumer audit identifies the first
  implementation owner and proof surface.
- Loaded-value and store-source freshness for global or pointer-value memory
  are deferred because address/range authority does not settle value
  freshness ownership.
- Aggregate-adjacent branch, select, call, publication, and non-branch
  pointer/address consumers remain separate use-specific routes.
- Relocation/materialization-only and target-local operand-shape routes do
  not need their own semantic implementation unless a later audit finds a
  concrete support-as-authority bug.

Impact on idea 591:

- Idea 591 should consume this research package as a boundary contract for
  `PreparedMirView`.
- The view may expose selected freshness authorities, semantic GEP
  availability, prepared support facts, target-consume facts, route proofs,
  diagnostic facts, and unavailable/deferred statuses.
- The view must not synthesize a generic pointer/address validity bit or
  infer semantic authority from complete stack homes, local layout/range
  facts, relocation/materialization records, target operands, diagnostics,
  dump shape, or candidate counts.
- Idea 591 should cite this package before choosing pointer/address view
  fields and should preserve unresolved families as unavailable or deferred
  until their first owner and proof surface are settled.
