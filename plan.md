# Inline-Assembly MIR-Ready BIR Allocation Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Activated from: the deferred scope left after the completed structured
LIR-to-BIR transport runbook

## Purpose

Repair and accept the architecture checkpoint, then build the target-prepared,
BIR-allocated path from verified Canonical BIR to MIR and late assembly.

## Goal

Publish a verified read-only `MirReadyBirView` over one immutable allocated BIR
revision in which every allocatable value has an abstract assignment and
ordinary pressure has already produced explicit abstract spill/reload nodes.

## Core Rule

Canonical BIR remains target-independent; target preparation supplies verified
capacity and constraint facts; BIR owns normal allocation and spilling; MIR
selects target instructions and maps abstract assignments to concrete ABI
registers without becoming a second allocator.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `docs/inline_asm_transport/index.md`
- `docs/inline_asm_transport/03_schema_checkpoint.md`
- `src/backend/bir/pipeline/README.md`
- `src/backend/bir/preparation/README.md`
- `src/backend/bir/preparation/inline_asm/README.md`
- `src/backend/bir/target_layout/README.md`
- `src/backend/bir/regalloc/README.md`
- `src/backend/bir/regalloc/constraints/README.md`
- `src/backend/bir/regalloc/spill_reload/README.md`
- `src/backend/bir/allocated/README.md`
- `src/backend/bir/REVIEW_TEMPLATE.md`

## Current Checkpoint

- Commit `0d55ee766` completed structured ordinary-SSA inline-asm transport
  through LIR-to-BIR; commit `24faa7516` recorded its full-suite proof and
  commit `bf3234f45` reconciled its scoped BIR READMEs.
- `docs/inline_asm_transport/03_schema_checkpoint.md` still describes the
  superseded route in which LIR-to-BIR normalizes target constraints and MIR
  owns virtual-register allocation. It is not implementation authority for
  this runbook until Step 1 repairs it.
- The BIR target-layout, preparation, regalloc, spill/reload, and allocated
  directories are scaffolds. Their ownership summaries broadly match the
  source idea, but no jointly reviewed end-to-end revision/token/verifier
  contract authorizes implementation yet.

## Current Targets

- one authoritative Canonical-BIR-to-MIR ownership and publication contract
- reviewed RV64, AArch64, and x86 abstract register-pool descriptors
- revision-bound inline-asm constraint and clobber preparation
- shared BIR allocation with abstract assignments and explicit spill/reload
- `PreparedBir` capability and `MirReadyBirView` over one allocated revision
- MIR selection and concrete ABI register mapping
- late inline-assembly substitution, parsing, and encoding

## Non-Goals

- Do not redo or weaken the completed structured LIR-to-BIR transport.
- Do not implement code before Step 1 architecture acceptance.
- Do not place target facts, concrete register identities, target opcodes, or
  frame offsets in Canonical or MIR-ready BIR nodes.
- Do not create a second instruction graph inside `PreparedBir` or a view.
- Do not make MIR or a target backend the normal allocator or pressure-spill
  owner.
- Do not parse asm mnemonics, placeholders, directives, `.insn`, or encoding
  details before late assembly.
- Do not restore `src/backend/legacy/**`, old prealloc/MIR, removed `c4c-as`,
  or deleted `src/backend/bir/mir/**` documents.
- Do not resume idea 730 globals work or general instruction-family lowering.

## Working Model

1. Verified Canonical BIR carries ordinary operands/results and the original
   opaque asm/constraint payload, ordered clobbers, and effect flags.
2. Target preparation reads that immutable revision with a selected target and
   publishes verified, revision-bound pool, ABI, class/group, tie,
   early-clobber, and resolved-clobber facts.
3. Shared BIR allocation consumes those facts and transactionally publishes a
   new immutable BIR revision with abstract `(category, class/group, slot)`
   assignments and explicit abstract `Spill`/`Reload` nodes.
4. `PreparedBir` is a capability over that revision and fact bundle.
   `MirReadyBirView` is its read-only BIR-to-MIR interface, not another IR.
5. MIR selects target instructions and maps abstract assignments to concrete
   RV64, AArch64, or x86 ABI registers. Any backend spill is a bounded final
   legalization fallback and cannot conceal ordinary capacity pressure.
6. Late assembly substitutes completed assignments and is the first stage to
   parse the opaque asm text.

## Execution Rules

- Step 1 is documentation, architecture repair, and independent review only.
  Steps 2-8 remain unauthorized until its completion check passes.
- Use ordinary instruction operands/results throughout. A read/write operand
  has an incoming use and distinct produced result; a tie equates assignments,
  never SSA identities.
- Bind every layout, preparation, allocation, view, and MIR-construction fact
  to the exact BIR revision and target-context identity/version.
- Publish each stage transactionally. A stale, incomplete, mismatched, or
  invalid fact bundle must publish no partial revision or capability.
- Freeze a closed MIR-ready abstract-node admission table. The examples in the
  source idea are not an implicit exhaustive list.
- Apply one legality/capacity model to normal allocation, retries, eviction,
  groups, fallback, spill, and reload.
- Keep the RV64 evidence boundary at `r`, `=r`, `VR`, `VRM2`, `VRM4`, and
  `VRM8`, including read/write, ties, early-clobbers, and clobbers. Reject
  `VRM1` and unreviewed forms.
- Use supervisor-selected proof commands and record exact results in
  `todo.md`. Never downgrade supported expectations or add named-case logic.

## Step 1: Repair and accept the MIR-ready allocation boundary

Goal: replace the superseded checkpoint with one coherent, reviewable contract
from Canonical BIR through target preparation, allocated-revision publication,
`MirReadyBirView`, MIR construction, and late assembly.

Primary targets:

- `docs/inline_asm_transport/03_schema_checkpoint.md`
- `docs/inline_asm_transport/index.md`
- the ownership contracts named under `Read First`

Concrete actions:

- Remove LIR-to-BIR target normalization, BIR target provenance, MIR virtual
  allocation, and MIR-owned ordinary spilling from the checkpoint.
- Freeze the closed MIR-ready abstract-node admission table and the rejection
  rules for target opcodes, concrete registers, frame offsets, unassigned
  allocatable values, and inconsistent spill/reload state.
- Define target-supplied abstract categories/classes, caller-saved,
  callee-saved, and temp capacities, reserved slots, aliases, ABI eligibility,
  group width/alignment/contiguity, ties, early-clobbers, and resolved clobber
  units for RV64, AArch64, and x86.
- Define the abstract assignment and spill-slot identities, capacity-driven
  BIR allocation/spill transaction, new immutable revision, exact revision
  binding, staleness rules, diagnostics, and failure publication behavior.
- Define `PreparedBir` as a capability and `MirReadyBirView` as a read-only
  view over the same allocated revision plus its verified typed facts.
- Define MIR input verification, target selection, concrete ABI mapping,
  revision trace, output verification, and transactional publication without
  inventing a replacement BIR-owned MIR filesystem.
- State the narrow invariant for final backend legalization spill/reload and
  the proof that it cannot mask BIR pool exhaustion or missing BIR spills.
- Preserve opaque `InlineAsm` payload and ordinary operand/result identities;
  type constraints only in preparation and parse instructions only in late
  assembly.
- Reconcile every named ownership README with the authoritative checkpoint in
  the same packet; do not claim scaffold APIs as implemented.
- Complete `src/backend/bir/REVIEW_TEMPLATE.md` against the repaired adjacency
  and obtain independent reviewer acceptance before implementation begins.

Completion check:

- One reviewed contract has no open owner, stage-token, revision, verifier,
  diagnostic, capacity, spill, fallback, or publication ambiguity.
- The reviewer records acceptance with no blocking source-idea drift or
  testcase-overfit finding. Only then may the supervisor advance to Step 2.

## Step 2: Implement verified target-layout and preparation facts

Goal: publish every immutable target fact required by allocation without
mutating or target-normalizing Canonical BIR.

Primary targets:

- `src/backend/bir/target_layout/**`
- `src/backend/bir/preparation/**`
- focused preparation and target-layout tests selected by the supervisor

Concrete actions:

- Implement reviewed RV64, AArch64, and x86 pool/capacity descriptors and
  target-context fingerprints.
- Type inline-asm roles, admitted classes/groups, ties, early-clobbers,
  effects, and clobber units against ordinary ordered operands/results.
- Reject stale revisions, target mismatch, impossible pools/groups,
  unsupported syntax, and `VRM1` with owner-specific diagnostics.
- Prove the Canonical BIR revision and original payload remain unchanged.

Completion check:

- Verified preparation exposes all and only the facts required by shared BIR
  allocation, and no downstream consumer must reinterpret source strings.

## Step 3: Freeze and verify the allocated abstract-BIR schema

Goal: establish the typed assignment, spill/reload, and admission model before
the allocation algorithm depends on it.

Primary targets:

- `src/backend/bir/core/**`
- `src/backend/bir/allocated/**`
- `src/backend/bir/verify/**`
- focused builder/view/verifier tests selected by the supervisor

Concrete actions:

- Add the reviewed abstract assignment and abstract spill-slot records without
  concrete register or frame identity.
- Add admitted abstract `Spill`/`Reload` nodes and immutable views/builders for
  the closed MIR-ready node table.
- Verify assignment completeness and legality, group slots, revision/fact
  binding, spill-slot consistency, placement, dominance, and reload coverage.
- Reject forbidden nodes and partial or stale publication transactionally.

Completion check:

- Direct negative tests fail every incomplete, illegal, stale, or forbidden
  graph without publishing a MIR-ready capability.

## Step 4: Implement shared BIR allocation and spill/reload

Goal: make BIR the sole normal owner of capacity allocation and pressure
spilling for all three targets.

Primary targets:

- `src/backend/bir/regalloc/**`
- shared liveness/interference analysis only where the accepted contract needs
  a narrow extension
- focused scalar, group, tie, clobber, pressure, and spill tests

Concrete actions:

- Assign every allocatable value an abstract category/class/group/slot under
  the verified capacities and ABI rules.
- Enforce ties, early-clobbers, aliases, reserved slots, liveness, interference,
  and atomic group allocation without merging SSA identities.
- Use the same legality filter in normal, retry, eviction, spill, and fallback
  routes.
- Insert explicit abstract spills/reloads for capacity pressure and publish one
  new immutable allocated revision transactionally.

Completion check:

- Scalar and group pressure tests prove complete legal assignments and
  explicit consistent spill state; no ordinary exhaustion reaches MIR.

## Step 5: Publish `PreparedBir` and `MirReadyBirView`

Goal: expose the exact verified allocated revision through a narrow,
read-only BIR-to-MIR capability.

Primary targets:

- `src/backend/bir/allocated/**`
- the accepted external preparation/publication seam
- focused capability, lifetime, mismatch, and staleness tests

Concrete actions:

- Publish `PreparedBir` without copying or rebuilding an instruction graph.
- Expose only the accepted `MirReadyBirView` graph and revision-bound target,
  preparation, assignment, and spill facts.
- Reject missing facts, revision/target mismatch, forbidden nodes, incomplete
  assignments, and inconsistent spill state before capability publication.

Completion check:

- Tests prove view identity/lifetime and fail closed for every cross-revision,
  cross-target, incomplete, or stale bundle.

## Step 6: Construct MIR and map concrete ABI registers

Goal: select target instructions from verified MIR-ready BIR without rerunning
ordinary allocation.

Concrete actions:

- Require verified `MirReadyBirView` at the MIR construction boundary.
- Select target instructions and deterministically map abstract assignments to
  concrete RV64, AArch64, and x86 ABI registers.
- Preserve inline-asm payload, operand/result order, and distinct identities;
  consume typed prepared facts rather than constraint strings.
- Implement only the reviewed bounded final legalization spill/reload fallback
  and reject its use for ordinary capacity exhaustion or missing BIR state.
- Publish verified MIR transactionally with exact revision trace.

Completion check:

- Concrete mappings are ABI-legal and deterministic, and instrumentation/tests
  prove MIR never performs ordinary allocation or pressure spilling.

## Step 7: Add the late assembler seam

Goal: make late assembly the first parser of inline-assembly instruction text.

Concrete actions:

- Accept opaque payload plus completed concrete operand assignments.
- Substitute the reviewed placeholder grammar, then parse mnemonics,
  directives, `.insn`, and encoding details.
- Prove malformed payload survives every earlier stage byte-for-byte and fails
  only at late assembly with owner-specific diagnostics.

Completion check:

- No earlier stage parses instruction text, and valid admitted payload encodes
  only after completed assignment mapping.

## Step 8: Prove the route and reconcile documentation

Goal: accept the bounded end-to-end route and leave implementation documents
truthful.

Concrete actions:

- Exercise scalar, read/write, tied, early-clobber, clobber, pressure/spill,
  calling-convention, and admitted RV64 vector-group cases end to end.
- Verify payload equality, revision binding, abstract assignments, explicit
  spills/reloads, concrete mappings, substitution, and owner-specific negative
  failures.
- Confirm build metadata excludes legacy/prealloc/old-MIR sources and run the
  supervisor-selected broader regression guard.
- Reconcile `src/backend/bir/core/README.md`,
  `src/backend/bir/lir_to_bir/README.md`, and
  `src/backend/bir/verify/README.md` against the final implementation.
- Record a closure audit in `todo.md` that enumerates remaining mismatches or
  explicitly states none, distinguishing intentional deferred scope from
  accidental desynchronization.

Completion check:

- The selected broad proof is green, review finds no target leakage, early
  parsing, missing BIR allocation, backend second allocation, expectation
  downgrade, or testcase-shaped shortcut, and the closure audit is explicit.
