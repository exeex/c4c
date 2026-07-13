# BIR Phase A Import And Raw Documentation Convergence Runbook

Status: Active
Source Idea: ideas/open/735_bir_phase_a_import_raw_document_convergence.md
Activated from: completed queue handoff in open umbrella idea 732

## Purpose

Converge the phase-A Markdown contracts from the complete current typed LIR
surface through private `ModuleDraft` construction and full Draft/Raw
verification to one move-only, verified, target-independent `RawBir` that the
accepted phase-B contract can consume exactly.

## Goal

Edit only the three phase-A-owned documents in strict `A1 -> A2` order and
prove the exhaustive 38-instruction, 6-terminator, 18-metadata-family intake,
receiving, verification, failure, and downstream handoff contract.

## Core Rule

This is a documentation-only runbook. Current LIR is complete, correct, and
immutable for this route. Receiving gaps belong to new-BIR documentation and
the deferred implementation consumer, not to LIR. No valid current source row
is complete merely because an unsupported diagnostic rejects it.

The current inline-assembly carrier already preserves ordinary values, exact
types, `Input`/`Output`/`ReadWrite` roles, constraint indices, original
constraint text, and opaque byte-preservable asm text. No LIR schema exception
is justified. Phase A documents only the missing Raw-BIR role/index receiving
owner, importer rule, verifier rule, and stable failure.

## Read First

- `ideas/open/735_bir_phase_a_import_raw_document_convergence.md`
- `ideas/open/732_bir_stage_document_convergence_umbrella.md` for the queue and
  final-audit lifecycle only
- `src/backend/bir/README.md`
- `src/backend/bir/lir_to_bir/README.md`
- `src/backend/bir/lir_to_bir/memory/README.md`
- `src/backend/bir/core/README.md`
- the Draft/Raw clauses of `src/backend/bir/verify/README.md`
- applicable clauses in `src/backend/bir/analysis/README.md`,
  `src/backend/bir/diagnostics/README.md`,
  `src/backend/bir/compatibility/README.md`,
  `src/backend/bir/LEGACY_COVERAGE.md`, and
  `src/backend/bir/REVIEW_TEMPLATE.md`
- `ideas/open/736_bir_phase_b_canonical_document_convergence.md` only as the
  exact downstream acceptance contract
- `ideas/open/734_lir_to_new_bir_container_completeness.md` only as the
  deferred inactive implementation consumer

## Owned Documentation Targets

Edit only these phase-A owners, in this order:

1. `src/backend/bir/lir_to_bir/README.md`
2. `src/backend/bir/lir_to_bir/memory/README.md`
3. `src/backend/bir/core/README.md`

Root, verifier, analysis, diagnostics, compatibility, coverage-ledger,
review-template, and phase-B documents are read-only adjacency evidence. If an
accepted repair must change a shared or adjacent owner, record the exact seam
and stop that packet until a coordinated documentation boundary explicitly
names both owners and the files it authorizes. Do not silently reassign or edit
shared authority.

## Required Matrix Inventory

The accepted phase-A input matrix must contain exactly one individually named
row for each current instruction alternative:

1. `LirInst::LirConstInt`
2. `LirInst::LirConstFloat`
3. `LirInst::LirLoad`
4. `LirInst::LirStore`
5. `LirInst::LirBinary`
6. `LirInst::LirCast`
7. `LirInst::LirCmp`
8. `LirInst::LirCall`
9. `LirInst::LirGep`
10. `LirInst::LirSelect`
11. `LirInst::LirIntrinsic`
12. `LirInst::LirInlineAsm`
13. `LirInst::LirMemcpyOp`
14. `LirInst::LirVaStartOp`
15. `LirInst::LirVaEndOp`
16. `LirInst::LirVaCopyOp`
17. `LirInst::LirStackSaveOp`
18. `LirInst::LirStackRestoreOp`
19. `LirInst::LirAbsOp`
20. `LirInst::LirIndirectBrOp`
21. `LirInst::LirExtractValueOp`
22. `LirInst::LirInsertValueOp`
23. `LirInst::LirLoadOp`
24. `LirInst::LirStoreOp`
25. `LirInst::LirMemsetOp`
26. `LirInst::LirCastOp`
27. `LirInst::LirGepOp`
28. `LirInst::LirCallOp`
29. `LirInst::LirBinOp`
30. `LirInst::LirCmpOp`
31. `LirInst::LirPhiOp`
32. `LirInst::LirSelectOp`
33. `LirInst::LirInsertElementOp`
34. `LirInst::LirExtractElementOp`
35. `LirInst::LirShuffleVectorOp`
36. `LirInst::LirVaArgOp`
37. `LirInst::LirAllocaOp`
38. `LirInst::LirInlineAsmOp`

It must also contain exactly one row for each terminator:

1. `LirTerminator::LirBr`
2. `LirTerminator::LirCondBr`
3. `LirTerminator::LirRet`
4. `LirTerminator::LirSwitch`
5. `LirTerminator::LirIndirectBr`
6. `LirTerminator::LirUnreachable`

And exactly one row for each metadata family:

1. `module-context`
2. `stable-identities`
3. `operand-kinds`
4. `type-system`
5. `functions-signatures`
6. `blocks-cfg-order`
7. `values-def-use`
8. `stack-objects-allocas`
9. `globals-objects`
10. `initializers`
11. `strings`
12. `externs`
13. `specializations`
14. `intrinsic-requirements`
15. `inline-asm-metadata`
16. `producer-indexes-caches`
17. `source-order-origin`
18. `module-publication`

The source idea's row tables are the durable field-level contract. Every
accepted row must name exact current LIR fields and authority class, one typed
Raw destination or explicit validation-only non-destination, importer rule,
current disposition, verifier owner/rule, stable failure, and positive plus
malformed/neighboring proof obligation. Old/new twins remain separate source
rows but converge on one semantic BIR owner; no catch-all or “remaining” row is
allowed.

## Target-Independent Raw Boundary

- `LirModule::target_profile` and rendered `data_layout` are audited
  validation/origin/parity-only source context with no semantic Raw
  destination. They cannot influence target-independent Raw facts. C1 later
  selects one exact `TargetProfile`; C2 derives target-layout facts.
- Raw BIR may retain opaque target-authored asm bytes and existing requirement
  tokens, but cannot interpret ABI placement, constraints, register classes,
  homes, frame state, target profiles/layout text, target opcodes, relocations,
  MIR, emission, or assembler syntax.
- Stable semantic identity comes only from typed stable IDs, never names,
  pointers, vector positions, render order, caches, or dense analysis indices.
- Every failure is module-transactional: validate before mutation where safe,
  poison/destroy unpublished state after mutation failure, preserve structured
  source-located diagnostics, and publish only the exact fully verified draft
  revision.

## Uniform Document Contract

Each owned document must carry the common metadata spine:
`Contract-Status`, `Implementation-Status`, `Kind`, `Phase-ID`, `Upstream`,
`Downstream`, `Owner-Path`, and `Last-Reconciled-Commit`. Its core-first order
is `Purpose`, `Owns`, `Does Not Own`, `Inputs`, `Outputs`, and
`Adjacent-Stage Contract`, followed by applicable ordered behavior,
invariants, verification/publication, failure/diagnostics,
analysis/invalidation, target/ABI rules, implementation state, proof, open
questions, and review checklist.

Heading insertion is not convergence. Every document requires an exhaustive
input matrix and output-handoff matrix with exact producer/consumer clauses,
revision/target binding, stable identities, validation and optional/error
forms, publication/verifier gate, failure behavior, invalidated analyses, and
checked implementation truth. Missing real owners may be documented as
indexed placeholders; a placeholder must not be called implemented.

## Non-Goals

- No C/C++ or other implementation, test, build, regression-log, runtime,
  expectation, unsupported-marker, allowlist, or behavior change.
- No LIR schema, producer, verifier, or source edit; no inline-asm carrier
  exception, special value model, binding table, parsing, allocation,
  projection, or assembler work.
- No canonicalization, target/profile selection, target preparation, ABI work,
  pseudo lowering, allocation, MIR, object, link, or emission work.
- No edit to idea 734 and no activation or implementation of it. It remains a
  deferred consumer after Child-A acceptance.
- No activation of Child B, draft idea 733, or any other lifecycle transition
  inside routine execution packets.
- No shared-owner edit without an explicitly coordinated documentation
  boundary; record the seam instead of broadening the packet.

## Execution Rules

1. Follow the three owned-document order exactly. Keep packet edits confined
   to the current owner unless the plan explicitly advances.
2. Treat the source idea's 38/6/18 tables as mandatory exact coverage, not
   examples. Adding a future source alternative cannot silently fall through;
   document closed dispatch tripwires and the maintained family checklist.
3. Classify current truth as already-proved coverage, stale documentation,
   missing new-BIR receiving container, missing importer wiring, or both
   container+wiring missing. Verify against checked-in/build-included code;
   build-excluded designs are not implementation.
4. Resolve ownership, full input coverage, output shape, and adjacent
   acceptance before API/algorithm/data-layout detail.
5. Never accept output by assertion. Cite the exact downstream clause that
   consumes every artifact and variant.
6. Preserve exact revision and target keys, deterministic order, optional and
   error forms, declaration/definition splits, forward references,
   duplicate/conflict behavior, invalidation, and failure atomicity.
7. Reject expectation weakening, supported-to-unsupported changes, helper
   renames, classification-only claims, rendered-text probes, named-case
   matchers, allowlists, and testcase-shaped shortcuts as convergence.
8. Update routine progress only in `todo.md`. Change this runbook only when its
   route or proof contract genuinely changes; change the source idea only when
   durable intent changes.

## Ordered Steps

### Step 1 - Converge A1 importer ownership and the exhaustive intake matrix

Goal: make the top-level importer document the lossless, closed, transactional
receipt of every current typed LIR fact.

Primary target: `src/backend/bir/lir_to_bir/README.md`

Actions:

- normalize the metadata and core-first sections before implementation detail
- replace stale source-gap assumptions with the exact 38/6/18 source and
  receiving matrix contract
- classify every field as semantic authority, compatibility mirror,
  producer/cache index, or validation evidence
- give every row one typed Raw destination or validation-only non-destination,
  importer rule, disposition, verifier/failure rule, and positive plus
  malformed/neighbor proof obligation
- preserve exact source/nested order, stable IDs, forward references,
  optional/error forms, declaration/definition splits and conflict behavior
- document closed variant-count and metadata-family maintenance tripwires
- preserve the no-LIR-edit and no-inline-asm-carrier-exception findings

Completion check:

- the importer document contains exactly 38 individually named instruction
  rows, six individually named terminator rows and 18 individually named
  metadata-family rows; every row has the required fields, no catch-all exists,
  and A1 failure publishes no draft, fixup table or partial capability

### Step 2 - Converge the A1 memory import sub-boundary

Goal: make the build-excluded memory document a truthful subordinate migration
boundary rather than a second importer, verifier, or implementation claim.

Primary target: `src/backend/bir/lir_to_bir/memory/README.md`

Actions:

- normalize its metadata, ownership, inputs, outputs and adjacency
- bind every memory-related phase-A matrix row to the top-level importer and
  exact Raw receiving owner without duplicating dispatch or publication
- preserve semantic memory/address/atomic facts while excluding target layout,
  address selection, scalarization, ABI placement and machine lowering
- state its build-excluded implementation truth and exact failure propagation
  through the sole full A2 Draft/Raw gate

Completion check:

- every memory row has one top-level importer path and one Raw owner, the file
  claims no independent verifier/publication or build coverage, and failure
  cannot emit an opaque placeholder or partial draft

### Step 3 - Converge the Raw core receiving and ownership contract

Goal: prove the core can describe one typed, deterministic, target-independent
receiving owner for every semantic phase-A row without claiming scaffolded
storage exists.

Primary target: `src/backend/bir/core/README.md`

Actions:

- normalize metadata and core-first ownership/input/output/adjacency sections
- reconcile the 38/6/18 destinations with the closed typed schema, stable ID
  families, ownership graph, deterministic iteration and exact def-use/CFG
- distinguish checked-in partial storage from missing target containers and
  remove stale implementation or source-gap claims
- keep target/profile/layout, ABI, constraints, homes, frames, MIR and emission
  out of Raw semantics
- define private `ModuleDraft` freeze and the single full A2
  `verify_and_publish_raw(ModuleDraft&&)` handoff without a builder bypass

Completion check:

- every semantic matrix row has exactly one typed core owner or an explicit
  truthful missing-container disposition; validation-only rows publish no
  semantic duplicate, all identities/orders are stable, and no target or
  allocation state enters Raw BIR

### Step 4 - Audit the shared A2 verifier and cross-cutting boundaries

Goal: prove the three owned documents agree with shared Draft/Raw,
analysis/invalidation, diagnostics, compatibility, coverage and review
contracts without silently reassigning those owners.

Actions:

- audit the shared Draft/Raw verifier clauses against every row, exact revision
  and publication/failure requirement
- audit analysis cache/revision keys, invalidation and stale-result rejection
- audit diagnostics as read-only rendering, compatibility as observational
  quarantine, and coverage/status statements against implementation truth
- record any necessary shared-owner change as a named coordinated
  documentation boundary with both owners and the exact seam; do not edit it
  in an unauthorized packet

Completion check:

- shared evidence accepts the exact A1/A2 contract and no duplicate verifier,
  semantic owner, cache identity or publication path remains hidden; if a
  shared-owner change is required, record the concrete coordinated boundary
  requirement and stop Step 4 until that separately authorized documentation
  repair is accepted

### Step 5 - Prove phase-B acceptance and the deferred idea-734 boundary

Goal: finish the phase-A documentation handoff without authorizing
implementation or activating downstream work.

Actions:

- build the exhaustive output-handoff matrices from A1 private draft through
  A2 Raw publication to phase B
- prove phase B accepts only one move-only verified `RawBir` whose current LIR
  facts have one typed owner, deterministic order and exact def-use/CFG
- reject importer maps, partial drafts, hidden side tables, unsupported valid
  rows, compatibility strings used as identity, target facts and allocation
  state at the B boundary
- record idea 734 only as the inactive implementation consumer of the accepted
  container/importer/verifier contract after Child-A acceptance
- run documentation structural, inventory, adjacency and diff checks; request
  plan-owner lifecycle judgment rather than activating B or 734 directly

Completion check:

- all three owned documents are substantively conformant and implementation-
  truthful; exact 38/6/18 input and complete output matrices pass structural
  checks; the A/B clauses agree; failure is atomic; LIR and idea 734 remain
  unchanged; and the accepted handoff authorizes documentation lifecycle only

## Runbook Completion

Runbook exhaustion does not itself close Child A. After Step 5, plan-owner must
decide from the source acceptance criteria and supervisor-owned documentation
proof whether Child A is complete, needs a bounded coordinated boundary
repair, or remains open. Umbrella idea 732 stays open through all six children
and its final cross-phase audit.
