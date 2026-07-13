# BIR Phase C Preparation Documentation Convergence Runbook

Status: Active
Source Idea: ideas/open/737_bir_phase_c_preparation_document_convergence.md
Activated from: accepted and closed Child B 736

## Purpose

Converge the external target-profile boundary, BIR target-layout derivation,
six immutable preparation products, cumulative preparation publication and
register-constraint binding without mutating the accepted `CanonicalBir`.

## Goal

Review phase C in exact `C1 -> C2 -> C3 -> C4 -> C5 -> C6 -> C7 -> C8 ->
C9` order and prove one exact target/profile/revision-keyed immutable bundle
accepted by D1.

## Core Rule

Phase C consumes only the exact verified, target-independent, unallocated
`CanonicalBir` accepted by closed Child B. C1 independently selects and
validates one exact `TargetProfile`; C2 derives BIR target-layout facts. Neither
Raw nor Canonical storage receives target/profile/layout semantics, and no
phase-C planner mutates either input revision.

Every product is immutable and keyed to the exact Canonical revision, target
fingerprint, planner/schema/options and dependency products. Mismatch,
staleness, planner failure or verifier failure publishes no partial product,
mixed bundle or readiness capability.

## Read First

- `ideas/open/737_bir_phase_c_preparation_document_convergence.md`
- `ideas/closed/736_bir_phase_b_canonical_document_convergence.md` as the
  accepted predecessor contract
- `ideas/open/738_bir_phase_d_pseudo_document_convergence.md` only as the
  downstream acceptance contract
- `src/backend/bir/README.md`
- `src/backend/bir/target_layout/README.md`
- `src/backend/bir/preparation/abi/README.md`
- `src/backend/bir/preparation/calls/README.md`
- `src/backend/bir/preparation/variadic/README.md`
- `src/backend/bir/preparation/address/README.md`
- `src/backend/bir/preparation/inline_asm/README.md`
- `src/backend/bir/preparation/runtime_helpers/README.md`
- `src/backend/bir/preparation/README.md`
- `src/backend/bir/regalloc/constraints/README.md`
- applicable exact-current call-graph, provenance,
  publication/value-flow, analysis-framework, verifier and diagnostics clauses

## Exact Review Order And Targets

1. C1: audit the external `TargetProfile` selection/validation authority and
   classify whether an explicit Markdown boundary/placeholder is required.
2. C2: `target_layout/README.md`.
3. C3: `preparation/abi/README.md`.
4. C4: `preparation/calls/README.md`.
5. C5: `preparation/variadic/README.md`.
6. C6: `preparation/address/README.md`.
7. C7: `preparation/inline_asm/README.md`.
8. C8: `preparation/runtime_helpers/README.md`, then
   `preparation/README.md` as C8 sequencing and atomic cumulative-publication
   support. It may be read earlier as context, but its owned review point is
   after the six leaf products.
9. C9: `regalloc/constraints/README.md`.
10. Final shared-boundary and D1 adjacency audit.

Request accepted shared analyses immediately before their earliest actual
consumer: publication/value-flow for ABI/result requirements, call-graph facts
for calls/helpers, and provenance for address preparation. They remain
immutable exact-revision dependencies and do not become phase-C stages or
product/publication owners.

## Uniform Document And Matrix Contract

Every owned document must carry `Contract-Status`, `Implementation-Status`,
`Kind`, `Phase-ID` or `Applies-To`, `Upstream`, `Downstream`, `Owner-Path`, and
`Last-Reconciled-Commit`. Stage/planner/boundary owners use the core-first order
`Purpose`, `Owns`, `Does Not Own`, `Inputs`, `Outputs`, and
`Adjacent-Stage Contract`, followed by applicable behavior, invariants,
verification/publication, failure/diagnostics, analysis/invalidation,
target/ABI rules, implementation state, proof, open questions and checklist.

Each document needs exhaustive input and output matrices naming the exact
Canonical/profile/product/revision keys, producer and consumer clauses, stable
IDs, validation and optional/error forms, publication/verifier gate, failure,
invalidation and checked implementation truth. A heading, status, scaffold,
unsupported diagnostic or design sketch is not implementation evidence.

## Target And Product Boundaries

- `target_profile` and rendered `data_layout` from source intake remain
  validation/origin/parity-only context with no semantic Raw/Canonical
  destination. C1 selects the exact profile from requested triple, arch, OS,
  ABI, relocation, float ABI and capabilities; C2 derives layout rather than
  importing allocator state or parsing Raw/Canonical layout text.
- C2 publishes verified finite pseudo categories, classes/groups, slots,
  aliases, reserved units, capacities, ABI eligibility and concrete-mapping
  domain keyed to the exact profile.
- C3-C8 publish immutable ABI, call, variadic, address, inline-asm vocabulary
  and runtime-helper products. C8 plus `preparation/README.md` atomically
  publishes one complete cumulative verified bundle.
- C7 supplies target constraint vocabulary/context/eligibility only. It does
  not parse or bind constraints. C9 alone parses, types and binds constraint
  descriptions to ordinary operands/results and publishes Canonical-keyed
  `BoundConstraintSet`.
- C9's subordinate `ConstraintProjectionTransaction` is the sole later-
  revision projection authority. It emits exact-revision
  `ProjectedConstraintSet` products only inside their enclosing later mutator;
  projection is not another phase-C stage.
- D1 receives accepted `CanonicalBir`, verified target layout, cumulative
  preparation products and typed constraints with exact matching keys. No
  target fact is written backward into Canonical storage.

## Implementation Truth And Shared Boundaries

Check every implementation status against checked-in storage, build inclusion,
callable planner/pipeline, verifier reachability and proof. Truthfully mark
absent, partial and scaffolded owners; documentation acceptance cannot claim
target preparation exists.

C1 is external authority, not a license to invent BIR implementation. The
accepted missing-owner audit in `869388429` authorizes only the coordinated
documentation boundary named in Step 1. Other shared root/verifier/analysis/
diagnostic or adjacent D1 edits still require a separate exact two-sided
coordinated boundary in this runbook; do not silently reassign them.

## Non-Goals

- No C/C++ or other code, tests, build files, regression logs, runtime
  behavior, expectations, unsupported markers or allowlist changes.
- No mutation of Raw/Canonical, target facts written into them, or parsing of
  rendered `data_layout` to manufacture semantics.
- No user target choice, C1 implementation invention, canonicalization, D1
  pseudo lowering, ABI call transport, allocation, spill/reload, frame, MIR,
  object, link or emission work.
- No C7 constraint parsing, duplicate constraint/projection authority, mixed-
  key product or partial cumulative publication.
- No activation of Child D, idea 734 or any lifecycle transition inside
  routine execution packets.

## Execution Rules

1. Follow the exact order. Do not review `preparation/README.md` as an owned
   convergence target before the six C3-C8 leaf products.
2. Resolve owner, inputs, outputs and adjacent acceptance before API,
   algorithm or data-layout detail. Create no implementation while documenting
   a missing owner/placeholder.
3. Bind every product to exact Canonical/profile/planner/dependency keys;
   reject stale, mixed or predecessor-keyed products atomically.
4. Request shared analyses at their earliest real consumer and preserve their
   read-only, exact-revision, invalidation/stale-result rules.
5. Keep C7 vocabulary separate from C9 interpretation/binding and C9
   projection separate from enclosing later semantic stages.
6. Reject assertion-only handoffs, status-only changes, unchecked
   implementation claims, expectation weakening, helper renames,
   classification-only progress, rendered-output probes, named-case matchers,
   allowlists and testcase-shaped shortcuts.
7. Update routine progress in `todo.md`; change this runbook only for a real
   route/proof correction and the source only for durable intent.

## Ordered Steps

### Step 1 - Resolve the external C1 Markdown boundary and Canonical input

Goal: establish the exact external target-profile selection authority and bind
it to the accepted Canonical input without inventing implementation.

Authorized writable set for one coordinated documentation-only packet:

- create `src/target_profile/README.md` as the sole external C1 target-request
  selection, normalization and validation owner
- repair only the required two-sided authority and links in
  `src/backend/bir/README.md`
- repair only the accepted B8/C1 downstream authority and links in
  `src/backend/bir/pipeline/README.md`
- repair only the C1/verifier binding authority and links in
  `src/backend/bir/verify/README.md`
- repair only the C1/C2 input authority and links in
  `src/backend/bir/target_layout/README.md`
- repair only the C1 producer authority and links in
  `src/backend/bir/preparation/README.md`

These six paths are the complete documentation edit set. The executor may
additionally update only `todo.md` for canonical Step-1 progress and proof; no
other path is writable. The shared pipeline and verifier documents are
accepted boundaries, so all six documentation paths must be changed and
re-audited as one coherent slice.

Actions:

- create the external C1 owner with truthful partial-foundation implementation
  status, core-first ownership clauses and exhaustive input/output matrices for
  requested triple, arch, OS, ABI, relocation, float ABI, capabilities and
  selection/schema versions
- make C1 alone select, normalize and validate the explicit request, then
  publish one immutable `TargetProfile` plus one complete
  `TargetFingerprint`; C1 cannot choose a target for the user
- preserve the shared verifier as the sole authority that binds the validated
  profile/fingerprint to the exact Canonical `PipelineStageStamp` and produces
  `VerifiedPreparationInput`
- preserve C2 as the sole target-layout derivation authority; C1 cannot parse
  rendered `data_layout`, derive layout or run C3-C9
- prove C1 consumes only the accepted exact `CanonicalBir` plus the explicit
  request, mutates no BIR and publishes nothing on validation failure
- place publication/value-flow, call-graph or other accepted analyses only at
  their later earliest consumers, not in C1 authority
- after the coordinated edits, re-audit all six paths for exact ownership,
  two-sided links, input/output matrices, implementation truth, failure
  atomicity and diff scope; keep Step 1 current until this re-audit passes

Completion check:

- all six authorized paths form one coherent boundary: C1 alone produces the
  validated profile/fingerprint, verifier alone produces exact-Canonical-bound
  `VerifiedPreparationInput`, and C2 alone derives layout; no implementation
  claim, BIR mutation, C1 analysis ownership, documentation edit outside the
  six-path set or noncanonical `todo.md` edit exists. Advance to Step 2 only
  after the post-packet re-audit proves this check.

### Step 2 - Converge C2 target-layout derivation

Goal: derive and verify one finite exact-profile-keyed BIR target layout.

Primary target: `src/backend/bir/target_layout/README.md`.

Actions:

- normalize metadata, core-first clauses and exhaustive input/output matrices
- enumerate all pseudo categories/classes/groups/slots/aliases/reserved units,
  capacities, ABI eligibility and concrete-mapping-domain facts
- define exact profile/schema/options keys, verifier, failure atomicity and C3
  consumer clauses
- reject imported allocator state, Raw/Canonical layout text and mutation of
  the accepted Canonical revision

Completion check:

- C2 publishes one verified immutable layout tied to exactly C1's profile key,
  every table has one authority, failure publishes nothing, and C3 accepts it

### Step 3 - Converge C3 ABI preparation

Goal: publish immutable typed parameter/result/byval/sret classification
requirements without performing call transport.

Primary target: `preparation/abi/README.md`; request publication/value-flow
facts at this earliest ABI/result consumer.

Actions:

- converge exact Canonical/profile/layout/analysis inputs and output matrices
- distinguish ABI classification requirements from concrete locations,
  lowering, homes and frame state
- define immutable product key, verifier, failure, invalidation and C4 handoff

Completion check:

- one exact-key `AbiPlan` captures requirements only, publishes atomically and
  is accepted by C4 without mutation or concrete placement

### Step 4 - Converge C4 call preparation

Goal: publish immutable typed call input/output, preservation, clobber and
return requirements without lowering calls.

Primary target: `preparation/calls/README.md`; request call-graph facts at this
earliest call/helper consumer.

Actions:

- bind every call form to exact Canonical/C1/C2/C3 and analysis keys
- separate planning requirements from D2's sole ABI-aware call-lowering
  authority
- define optional/error forms, verifier, failure, invalidation and C5 handoff

Completion check:

- one exact-key `CallPlan` covers every call form without transport or target
  opcode work and is accepted by C5

### Step 5 - Converge C5 variadic preparation

Goal: publish typed variadic entry, save-area, promotion and traversal
requirements without frame or ABI transport implementation.

Primary target: `preparation/variadic/README.md`.

Actions:

- enumerate fixed/variadic forms, promotions, save-area and traversal facts
- bind output to exact cumulative predecessor keys and define verifier/failure
- prove C6 accepts the immutable product without hidden layout or mutation

Completion check:

- all variadic variants have one typed requirement disposition and one exact-
  key immutable product reaches C6 atomically

### Step 6 - Converge C6 address preparation

Goal: publish typed address-materialization and relocation requirements without
target instruction selection or concrete frame placement.

Primary target: `preparation/address/README.md`; request provenance at this
earliest address consumer.

The current C6 packet established that the shared Provenance contract promises
later exact-revision address queries but normatively admits only B4. Before
completing the already-owned address contract, one narrow coordinated
documentation-only repair is authorized in
`src/backend/bir/analysis/provenance/README.md`.

The complete documentation edit set for this repair is exactly:

- `src/backend/bir/analysis/provenance/README.md`, then
- `src/backend/bir/preparation/address/README.md`.

The executor may additionally update only `todo.md` for canonical Step-6
progress and proof; no other path, analysis owner, code, build, test or log is
writable.

Actions:

- repair the Provenance input/stage contract to admit verified Canonical/B8 or
  an exact-later semantic request only with exact-current same-revision CFG,
  Dominance, PublicationValueFlow and MemoryEffects dependency closure
- retain B5 as the earliest mutation consumer and preserve canonical empty
  options, target key `None`, empty preparation, complete-key stale rejection,
  `Known`/`Absent`/`Unknown` results and absent implementation truth
- do not add analysis implementation or edit any other analysis/shared owner
- bind every address/global/local/relocation form to exact predecessor and
  fresh exact-B8 provenance keys; never reuse a B4 handle or silently refresh
  dependencies beneath an old request
- separate semantic requirements from D4 legalization, E4 frame placement and
  F1 one-to-one application
- define verifier, failure, invalidation and C7 handoff
- rerun the complete C6 seam proof after both documentation edits; remove the
  blocker and advance only when exact scope, dependency freshness and all
  address/consumer checks are green

Completion check:

- the narrow Provenance seam admits exact-current B8/later-semantic requests
  without weakening its B4 consumer, target/preparation exclusions, statuses
  or implementation truth; one immutable exact-key address product then covers
  every form, stale provenance fails, and C7 accepts the result. Keep Step 6
  current unless the post-repair seam proof is green.

### Step 7 - Converge C7 inline-assembly target vocabulary

Goal: publish only target constraint vocabulary, eligibility/context and
clobber tables while preserving opaque assembly and ordinary values.

Primary target: `preparation/inline_asm/README.md`.

Actions:

- enumerate vocabulary/context/eligibility/clobber inputs and exact outputs
- keep asm bytes opaque and ordinary operands/results unchanged
- forbid constraint parsing/binding, placeholder interpretation, allocation,
  projections and concrete register reservation
- define exact keys, verifier/failure and C8 handoff

Completion check:

- C7 publishes only immutable target tables, no constraint meaning is attached
  to operands, and C8 accepts the exact product

### Step 8 - Converge C8 helpers and cumulative preparation publication

Goal: finish the six preparation products and atomically publish one verified
cumulative bundle.

Primary targets in order: `preparation/runtime_helpers/README.md`, then
`preparation/README.md`.

Actions:

- converge helper eligibility/interface requirements against exact call/ABI
  and call-graph keys without lowering helpers
- review `preparation/README.md` only after C3-C8 leaves, preserving exact leaf
  order, dependency/product keys and transactional cumulative publication
- prove any leaf/key/verifier failure publishes no partial bundle or readiness
  capability

Completion check:

- C8 and sequencing support publish one complete verified cumulative bundle
  tied to exact Canonical/profile/layout/leaf keys and accepted by C9

### Step 9 - Converge C9 constraint binding and projection authority

Goal: bind target constraint descriptions to ordinary values exactly once and
define the sole later-revision projection authority.

Primary target: `src/backend/bir/regalloc/constraints/README.md`.

Actions:

- converge exact cumulative inputs, parser/type/binding rows, optional/error
  forms and Canonical-keyed `BoundConstraintSet` output
- keep C9 as the sole interpreter; no C7, pass, allocator or MIR duplicate
  parses or binds constraints
- define `ConstraintProjectionTransaction` keys, enclosing-transaction
  atomicity, invalidation and D1 projection/consumer clauses

Completion check:

- one immutable Canonical-keyed bound set and one sole exact-revision
  projection protocol are documented; failure publishes no partial/mixed-key
  product; D1 accepts the complete C bundle

### Step 10 - Prove D1 acceptance and Child-C completion

Goal: close all phase-C shared seams and prove the immutable preparation/
constraint handoff without authorizing pseudo implementation.

Actions:

- finalize exhaustive phase-C input/output and cross-document adjacency
  matrices
- audit shared analyses, root/verifier/diagnostics and D1 clauses against exact
  profile/revision/product keys; request a coordinated documentation boundary
  for any necessary shared-owner edit
- prove D1 accepts only accepted `CanonicalBir`, verified layout, cumulative
  preparation bundle and typed constraints with matching keys
- run documentation structure, order, link, matrix, implementation-truth,
  invalidation, failure and diff-scope checks
- request plan-owner completion judgment; do not activate Child D directly

Completion check:

- C1-C9 ownership/order is exact, every product is immutable/current and
  failure-atomic, all shared and C/D adjacency checks pass, implementation
  truth is explicit, and the D1 handoff authorizes documentation lifecycle only

## Runbook Completion

Runbook exhaustion does not itself close Child C. After Step 10, plan-owner
must decide from source acceptance criteria and supervisor-owned documentation
proof whether Child C is complete, needs a bounded coordinated repair, or
remains open. Umbrella idea 732 stays open through Children C-F and its final
cross-phase audit.
