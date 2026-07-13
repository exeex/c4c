# BIR Phase B Canonical Documentation Convergence Runbook

Status: Active
Source Idea: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Activated from: accepted and closed Child A 735

## Purpose

Converge the target-independent canonicalization documentation from the exact
accepted phase-A `RawBir` through immutable P01-P07 revisions and the B8
Canonical publication boundary, producing one verified, target-independent,
unallocated `CanonicalBir` accepted by phase C.

## Goal

Review and edit phase-B Markdown in exact P01-P07 order with on-demand analyses
immediately before their earliest consumers, then converge B8 in exact pass-
framework, pipeline, shared-verifier order.

## Core Rule

Phase B consumes only the immutable exact-revision `RawBir` accepted by closed
Child A. It performs target-independent canonicalization only. It cannot repair
phase-A intake/publication failure, import target/profile/layout facts, select
ABI behavior, prepare targets, lower pseudos, allocate, build MIR, or emit.

Every pass occurrence is transactional. Failure publishes no partial revision,
property stamp, analysis cache entry or capability. Only B8's complete
Canonical verifier gate may mint `CanonicalBir` after every P01-P07
postcondition is current on the same frozen revision.

## Read First

- `ideas/open/736_bir_phase_b_canonical_document_convergence.md`
- `ideas/closed/735_bir_phase_a_import_raw_document_convergence.md` as the
  accepted predecessor contract
- `ideas/open/737_bir_phase_c_preparation_document_convergence.md` only as the
  downstream acceptance contract
- `src/backend/bir/README.md`
- `src/backend/bir/analysis/README.md`
- `src/backend/bir/passes/README.md`
- `src/backend/bir/pipeline/README.md`
- the Canonical clauses of `src/backend/bir/verify/README.md`
- the P01-P07 pass and earliest-consumer analysis documents named below

## Exact Review Order And Targets

Review phase-B owners in this order:

Before item 1, Step 1 may make the single authorized adjacency correction to
`passes/README.md` described in that step. This is not early pass-framework
convergence: P01 remains the first normative phase-B owner, and the framework's
full review remains at B8.

1. B1/P01: `passes/legalize/README.md`.
2. `analysis/comparison/README.md`, then B2/P02
   `passes/scalar/README.md`.
3. `analysis/cfg/README.md`, then B3/P03 `passes/cfg/README.md`; recompute CFG
   from the resulting terminators afterward.
4. `analysis/dominance/README.md` and
   `analysis/publication/README.md` at their earliest applicable SSA consumer,
   then B4/P04 `passes/ssa/README.md`.
5. `analysis/memory_effects/README.md` (available from Raw) and
   `analysis/provenance/README.md` once CFG/dominance/typed-value prerequisites
   exist, then B5/P05 `passes/memory/README.md`.
6. B6/P06 `passes/aggregate/README.md`.
7. `analysis/call_graph/README.md` at its earliest intrinsic/helper consumer,
   then B7/P07 `passes/intrinsics/README.md`; reuse or recompute memory effects
   according to its exact invalidation key.
8. B8: `passes/README.md` as pass-framework and orchestration support;
   `pipeline/README.md` as the ordered pipeline and capability boundary; then
   the shared verifier's complete `Canonical` publication gate.

`analysis/README.md` owns descriptors, exact revision keys, dependency caches,
preservation/invalidation and stale-result rejection. Analyses are immutable
on-demand dependencies, not extra linear passes. Root/core/diagnostic/support
clauses are adjacency evidence and retain their own authority.

## Uniform Document And Matrix Contract

Each reviewed owner must carry the common metadata spine:
`Contract-Status`, `Implementation-Status`, `Kind`, `Phase-ID` or
`Applies-To`, `Upstream`, `Downstream`, `Owner-Path`, and
`Last-Reconciled-Commit`. Stage/pass/boundary owners use the core-first order
`Purpose`, `Owns`, `Does Not Own`, `Inputs`, `Outputs`, and
`Adjacent-Stage Contract`, followed by applicable behavior, invariants,
verification/publication, failure/diagnostics, analysis/invalidation,
target/ABI rules, implementation state, proof, open questions and checklist.

Every owner needs substantive input and output matrices. Each meaningful form
or product row names its exact producer and consumer clause, revision key,
stable IDs, validation and optional/error forms, publication/verifier gate,
failure behavior, invalidated analyses and checked implementation truth.
Analysis variants additionally name cache/dependency keys, earliest consumer,
derived facts, invalidation and stale-result rejection. Heading or status edits
alone are not convergence.

## Exact Input And Output Boundaries

- Input is exactly one accepted move-only, verified, target-independent,
  unallocated `RawBir` revision. No draft, importer map, compatibility side
  table, unsupported valid row, hidden target context or allocation state is
  admitted.
- P01-P07 identities and order are immutable. Each pass consumes the exact
  accepted predecessor revision and publishes one immutable successor or
  fails atomically. Every Raw-only form has one named owner and every
  cumulative normal-form property is explicit.
- Terminators remain the sole CFG-successor authority; typed stable IDs remain
  semantic identity; names, pointers, vector positions, rendered text and
  dense analysis indices cannot replace them.
- Output is one exact-revision `CanonicalBir` that satisfies all Raw rules and
  every P01-P07 postcondition while remaining target-independent and
  unallocated. Phase C receives no Raw alias, stale analysis, compatibility
  identity, ABI placement, register home or spill state.

## Implementation Truth And Shared Boundaries

Check each status against current storage, build inclusion, callable pipeline,
verifier reachability and proof. A scaffold, design-only owner, partial route,
unsupported diagnostic or status label is not complete implementation.

Edit the phase-B pass/analysis/framework/pipeline owners assigned by this
runbook. The Canonical verifier, root, core and other cross-cutting owners are
shared or adjacent. If convergence requires changing one, record the exact
two-sided seam and stop until the runbook explicitly authorizes a coordinated
documentation boundary naming every file. Do not silently reassign authority
or broaden a pass packet.

Step 1 contains one such closed authorization: before legalize convergence,
edit only the contradictory **Allowed dependency direction** allowance in
`passes/README.md`. This early adjacency repair does not authorize its full B8
review, which remains Step 8, and does not authorize root, core, verifier or
closed Child-A edits.

## Non-Goals

- No C/C++ or other code, tests, build files, regression logs, runtime
  behavior, expected output, unsupported marker or allowlist change.
- No phase-A import/core repair and no edit to closed Child A or deferred idea
  734.
- No target/profile/layout selection, ABI or preparation work, constraint
  binding, pseudo lowering, call lowering, allocation, spill/reload, frame,
  MIR, object, link or emission work.
- No P01-P07 reorder/renumber, hidden pass insertion, target-dependent
  canonical form, or analysis promoted to a semantic stage.
- No activation of Child C or any lifecycle transition inside routine
  execution packets.

## Execution Rules

1. Follow the exact review order. A later pass cannot begin until the previous
   output and its consumer clause agree.
2. Request analyses immediately before their earliest actual consumer. After a
   mutation, invalidate/recompute unless exact preservation is proved.
3. Preserve the distinction between pass framework (transactions, occurrences,
   analysis access, invalidation and orchestration support), pipeline (ordered
   configured sequence and capability boundary) and verifier (Canonical
   acceptance/publication).
4. Resolve ownership, input coverage, output shape and adjacency before API,
   algorithm or data-layout detail. Detect and index missing real owners or
   documentation placeholders without calling them implemented.
5. Reject assertion-only handoffs, stale/mixed keys, partial publication,
   unchecked implementation claims, expectation weakening, helper renames,
   classification-only progress, rendered-text probes, named-case matchers,
   allowlists and testcase-shaped shortcuts.
6. Update routine progress in `todo.md`. Change this runbook only for a real
   route/proof correction and the source idea only for durable intent.

## Ordered Steps

### Step 1 - Establish the phase-B baseline and converge B1 P01 legalize

Goal: bind phase B to the accepted immutable Raw input and converge the sole
P01 ownership/disposition contract.

Primary targets: `src/backend/bir/passes/legalize/README.md` and read-only
baseline evidence in the root, core, verifier, closed Child A and B8 owners.
Before editing legalize, this step also authorizes exactly one narrow shared-
document repair in `src/backend/bir/passes/README.md`.

Actions:

- verify the exact accepted `RawBir` input, current implementation status,
  phase-B owner inventory and P01-P07/B8 order
- in `passes/README.md` only, replace the contradictory allowance that
  target-independent BIR may include an already-resolved module data layout;
  state instead that Raw/Canonical may preserve source-semantic typed sizes,
  alignments and address spaces owned by their accepted rows, but contain no
  semantic `target_profile`, rendered `data_layout`, target triple,
  pointer-width/address-space layout selection or other C1/C2 target context;
  C1 selects the exact `TargetProfile` and C2 derives target layout
- keep every other `passes/README.md` clause for its full B8 convergence in
  Step 8; re-audit the accepted Raw boundary after the narrow edit, and do not
  edit the root, core, verifier or closed Child A
- normalize P01 metadata, core-first clauses and exhaustive input/output matrix
- assign each Raw-only legalize form exactly one P01 disposition and explicit
  failure; B1 may inherit A2 failure but cannot repair the producer
- define exact revision, transaction, invalidation, cumulative postcondition
  and B2 consumer clauses

Completion check:

- the narrow framework clause agrees with the accepted target-independent Raw
  boundary without changing any other shared owner; the B1 owner consumes only
  that immutable Raw revision; every P01 form has one lossless
  normalize/preserve/reject disposition; failure publishes nothing; and the
  exact P01 output is accepted by B2

### Step 2 - Converge comparison analysis and B2 P02 scalar

Goal: place comparison/select facts at their earliest consumer and converge
scalar, comparison and select normalization.

Primary targets: `analysis/comparison/README.md`, then
`passes/scalar/README.md`.

Actions:

- define the analysis descriptor, dependencies, exact key, immutable result,
  non-provable/error forms and invalidation
- make P02 consume the exact P01 revision plus matching analysis facts
- enumerate closed scalar/comparison/select dispositions, transaction,
  postconditions, failure and B3 handoff

Completion check:

- analysis facts are same-revision and non-authoritative, P02 owns every named
  normalization without target interpretation, and one exact output reaches B3

### Step 3 - Converge CFG analysis and B3 P03 CFG

Goal: normalize terminators, blocks and edges while retaining terminators as
the sole stored successor authority.

Primary targets: `analysis/cfg/README.md`, then `passes/cfg/README.md`.

Actions:

- bind pre-planning CFG facts to the exact P02 revision and define stale/failure
  behavior
- converge P03's closed CFG transformations, stable identity/order, transaction
  and invalidation contract
- require CFG recomputation from the resulting terminators before downstream
  dominance/SSA use

Completion check:

- P03 publishes one verified exact revision with normalized CFG, no cached edge
  becomes semantic authority, and recomputed CFG is ready for B4 dependencies

### Step 4 - Converge dominance/publication analyses and B4 P04 SSA

Goal: place exact dominance and value-publication facts before their earliest
SSA consumer and converge canonical SSA/phi form.

Primary targets: `analysis/dominance/README.md`,
`analysis/publication/README.md`, then `passes/ssa/README.md`.

Actions:

- define exact CFG dependencies, immutable facts, failure and invalidation for
  both analyses
- converge P04 ownership of definitions, uses, phi edges and canonical SSA
  without name/position inference
- prove transaction rollback, cumulative postconditions and B5 acceptance

Completion check:

- exact-current analyses feed one P04 transaction, SSA/phi authority is unique,
  failure publishes no repair/cache/revision, and B5 accepts the output

### Step 5 - Converge memory/provenance analyses and B5 P05 memory

Goal: normalize target-independent memory, address, atomic and effect-bearing
forms using exact-current facts.

Primary targets: `analysis/memory_effects/README.md`,
`analysis/provenance/README.md`, then `passes/memory/README.md`.

Actions:

- bind memory effects to Raw/current revisions and provenance to its typed
  value/CFG/dominance prerequisites
- converge P05's closed form table, preservation/invalidation, transaction,
  failure and B6 handoff
- forbid ABI/address selection, target layout, scalarization by guesswork or
  compatibility-text recovery

Completion check:

- exact-current facts justify every P05 rewrite/preservation, stale facts fail,
  and one target-independent exact revision reaches B6

### Step 6 - Converge B6 P06 aggregate

Goal: converge aggregate values, copies and projections without introducing
target policy or weakening cumulative normal forms.

Primary target: `passes/aggregate/README.md`.

Actions:

- normalize metadata/core/matrices and enumerate the closed aggregate forms
- preserve exact stable identities, types, transaction and invalidation
- prove cumulative P01-P06 postconditions and B7 acceptance

Completion check:

- every aggregate form has one disposition, failure is atomic, and B7 receives
  one exact revision satisfying P01-P06

### Step 7 - Converge call-graph dependency and B7 P07 intrinsics

Goal: place call-graph facts at their earliest intrinsic/helper consumer and
produce the sole candidate eligible for B8.

Primary targets: `analysis/call_graph/README.md`, then
`passes/intrinsics/README.md`; audit/reuse memory effects by exact key.

Actions:

- define call-graph descriptor, call-semantics facts, exact key, failure and
  invalidation
- converge P07's closed target-independent intrinsic dispositions and preserve
  `InlineAsm` as one opaque ordinary-value node
- rebind/recompute memory effects when required and prove cumulative P01-P07
  properties, rollback and B8 input acceptance

Completion check:

- P07 publishes one exact candidate with all canonical normal forms, matching
  analysis products and no target/ABI interpretation; B8 accepts it exactly

### Step 8 - Converge B8 framework, pipeline and Canonical publication

Goal: separate framework, pipeline and verifier authority and publish exactly
one verified `CanonicalBir`.

Primary targets in order: `passes/README.md`, `pipeline/README.md`, then the
shared Canonical clauses of `verify/README.md`.

Actions:

- converge framework transaction/occurrence/analysis/invalidation and
  orchestration-support clauses without making it the ordered capability owner
- converge pipeline's exact P01-P07 sequence, stamps, re-entry, rollback and
  capability boundary
- audit the shared Canonical verifier against every cumulative postcondition;
  if its text must change, stop and request an exact coordinated documentation
  boundary before editing it
- prove only the full same-revision Canonical gate can mint `CanonicalBir`

Completion check:

- framework, pipeline and verifier authorities are distinct and adjacent;
  P01-P07 order is exact; failure publishes no mixed/partial state; and one
  target-independent unallocated `CanonicalBir` is published

### Step 9 - Prove phase-C acceptance and Child-B completion

Goal: complete the Canonical handoff without authorizing target-aware work.

Actions:

- build/finalize exhaustive phase-B input/output and cross-document adjacency
  matrices
- prove phase C accepts exactly the verified `CanonicalBir` plus no stale
  analysis, Raw alias, target fact, ABI placement, home or spill state
- run documentation structure, order, link, matrix, implementation-truth,
  invalidation, failure and diff-scope checks
- request plan-owner completion judgment; do not activate Child C directly

Completion check:

- every phase-B owner is substantively conformant and implementation-truthful;
  P01-P07/B8 and analysis order are exact; all adjacency and failure checks
  pass; and the phase-C handoff authorizes documentation lifecycle only

## Runbook Completion

Runbook exhaustion does not itself close Child B. After Step 9, plan-owner must
decide from source acceptance criteria and supervisor-owned documentation proof
whether Child B is complete, needs a bounded coordinated boundary repair, or
remains open. Umbrella idea 732 stays open through Children B-F and its final
cross-phase audit.
