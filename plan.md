# BIR Architecture Documentation Repair Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Supersedes: the documentation-convergence acceptance tail rejected by the
first full review, the failed Step 13 tail recorded in
`review/731_full_architecture_review_repeat.md`, and the repeated Step 13
checkpoint rejected by
`review/731_post_repair_full_architecture_review.md`; the current bounded reset
consumes `review/731_final_frame_action_architecture_review.md`

## Purpose

Repair the cross-document BIR architecture contradictions found by the full
Step 13 review, then repeat root reconciliation and independent review before
architecture acceptance.

## Goal

Make the root A-F registry and every subordinate BIR Markdown contract agree
on stage order, revision/profile ownership, copy realizability, constraint
projection, and legacy disposition without authorizing implementation.

## Core Rule

This runbook is docs-only. All target-specific expansion, parallel-copy
realization, abstract-home allocation, and ordinary pressure spill/reload must
finish in BIR before E4 publication. MIR and target backends may map verified
homes and select/encode instructions, but may not allocate, introduce
allocatable temporaries, spill/reload as repair, or weaken an E4 failure.

Implementation remains blocked, Step 14 remains forbidden, and idea 731 stays
open until every repair step is complete and a new independent full review has
no blocking finding.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `review/731_full_architecture_review_repeat.md`
- `review/731_post_repair_full_architecture_review.md`
- `review/731_final_frame_action_architecture_review.md`
- `src/backend/bir/README.md`
- `src/backend/bir/REVIEW_TEMPLATE.md`
- `src/backend/bir/LEGACY_COVERAGE.md`

## Accepted Checkpoint

- The root inventory currently accounts for all 44 BIR Markdown files: one
  root overview and 43 distinct subordinate paths.
- Raw and Canonical BIR remain target-independent and unallocated.
- C7 owns inline-asm target vocabulary/context tables; C9 alone parses, types,
  and binds original constraint text.
- `PreparedBir` is graphless and `MirReadyBirView` borrows the exact immutable
  E4 `AllocatedBir` revision.
- Diagnostics remain read-only and compatibility remains a non-authoritative
  quarantine.
- C9's immutable Canonical binding and sole shared projected-constraint
  authority remain resolved. The Step 9.2 frame-action repair reopened only
  the Step 10 adjacency for the final D5-private/E4-materialized projection
  invocation and key; no second binder or projection authority is permitted.
- The strict no-late-allocation-repair decision is accepted and must not be
  reopened as a shortcut around D5 or E4.

These facts are checkpoints, not architecture acceptance. Any repair that
changes an adjacent contract must recheck both sides and the root registry.

## Scope

- stale `S00`-`S29` and `G01` identifiers on the affected A-E surfaces
- unresolved core, pipeline, verifier-profile, and preparation/C9 boundary
  choices that currently contradict closed/converged claims
- a complete indexed subordinate D2 call-lowering contract
- a realizable BIR-owned path from D5 parallel-copy semantics to strict E4
  publication and one-to-one MIR consumption
- one explicit constraint-product lineage and re-projection owner for every
  mutating pseudo/allocated revision
- legacy/core ownership and disposition mismatches
- final root reconciliation, structural proof, and independent full review

## Non-Goals

- no C++ implementation, tests, fixtures, build metadata, or regression logs
- no target preparation, pseudo lowering, allocation, MIR, or assembly work
- no restoration of legacy/prealloc/old-MIR sources or documents
- no weakening of the strict E4/MIR boundary
- no named-case shortcut, expectation downgrade, or classification-only claim
  of capability progress
- no edit to the durable source idea unless a later review proves its intent is
  itself inconsistent; this reset does not establish such a conflict

## Working Model

The root README is the sole normative registry. Local documents define their
stage contracts but cannot invent predecessor/successor IDs or reorder the
root. The repair order starts at the first affected Step 2 surface and moves
forward so later profiles consume already-repaired names and products.

For constraint lineage, keep the immutable Canonical C9 binding distinct from
revision-specific projected bindings. Every mutating stage must either publish
the projection/preservation proof for its exact output revision through one
named shared projection authority or fail transactionally. Stable IDs alone
never prove freshness.

For D5, preserve simultaneous parallel-copy semantics only until a named
BIR-owned, allocation-aware resolution point. Before E4, cycles, overlap,
scratch requirements, and any required spill state must be represented by
directly realizable verified BIR nodes and abstract homes. MIR cannot expand a
bundle or invent a temporary.

## Execution Rules

1. For this review reset, execute Step 9.2 with its reopened Step 10 projection
   adjacency, then Steps 12 and 13 in order. Steps 2.1-9.1 and 11 are resolved
   checkpoints; all other Step 10 projection routes remain resolved. Do not
   create a broader repeat packet unless this bounded repair invalidates an
   adjacent contract.
2. Treat each substep as one bounded documentation packet. Compare all edited
   contracts with their immediate predecessor, successor, and root entry.
3. Replace stale identifiers mechanically only after checking the semantic
   predecessor, output profile, retry edge, and verifier gate represented by
   each occurrence.
4. Resolve architecture questions by choosing and documenting one contract.
   Do not relabel unresolved boundary/profile choices as deferred
   implementation.
5. Keep the Step 7 review order exactly:
   `target_layout -> preparation -> abi -> calls -> variadic -> address ->
   inline_asm -> runtime_helpers -> regalloc/constraints`.
6. A new D2 document must be added to the root inventory and all adjacent
   indexes in the same packet; inventory exactness is recalculated rather than
   preserving the old count by assumption.
7. Keep C9 as the sole original-constraint parser/binder and name exactly one
   projection mechanism plus the per-revision stage responsible for invoking
   it and publishing its output.
8. Do not solve D5 by moving out-of-SSA, allocation, scratch creation, or
   capacity spill/reload into MIR/backend. E4 must reject unresolved pressure
   and unrealizable nodes.
9. Use `src/backend/bir/REVIEW_TEMPLATE.md` for each packet and update
   `src/backend/bir/LEGACY_COVERAGE.md` when an owner/disposition changes.
10. For each packet, run `git diff --check`, link/path checks, and focused `rg`
    searches selected by the supervisor. Docs-only packets need no code build.
11. Record routine packet progress and exact proof in `todo.md`; do not rewrite
    this plan after every packet.
12. A blocker in Steps 2-12 returns execution to the earliest affected step.
    A blocker in Step 13 forbids Step 14 and requires another plan-owner reset
    or focused repair before the full review is repeated.

## Ordered Repair Steps

### Step 2.1 - Repair the A1 import order identifiers

Goal: make the earliest affected import sub-boundary use the root A-F registry.

Primary target:

- `src/backend/bir/lir_to_bir/memory/README.md`

Actions:

- replace stale `S00`/`S01` references with the exact A1/A2 predecessor,
  publication, and verifier vocabulary from the root
- cross-check `lir_to_bir/README.md`, `core/README.md`, and the Raw verifier
  profile without expanding the importer contract

Completion check:

- no removed registry identifier remains on the A1 import surfaces, and the
  memory sub-boundary agrees with Raw publication and failure atomicity

### Step 2.2 - Close core architecture choices and repair its BIR disposition

Goal: make A2 core a closed carrier contract without contradicting later BIR
allocation ownership.

Primary target:

- `src/backend/bir/core/README.md`

Actions:

- choose or explicitly assign the intrinsic, asm-goto, debug, unwind, SSA, and
  edge schema contracts required by adjacent accepted profiles
- distinguish intentionally unimplemented source carriers from unresolved
  architecture; only the former may remain deferred
- narrow the legacy disposition so interval/home/spill/reload state is absent
  from Raw/Canonical core storage but owned by later E1-E3 BIR contracts, not
  MIR/backend

Completion check:

- no acceptance-critical core schema is both marked open and consumed by a
  closed adjacent profile; later BIR allocation ownership is stated correctly

### Step 3.1 - Propagate the A-F registry through infrastructure

Goal: remove the obsolete registry from shared orchestration and framework
contracts without changing stage order.

Review in this order:

1. `src/backend/bir/pipeline/README.md`
2. `src/backend/bir/passes/README.md`
3. `src/backend/bir/analysis/README.md`

Actions:

- map every stale `S00`-`S29`/`G01` predecessor, successor, retry, and verifier
  reference to the exact root A-F stage/profile
- preserve the root as sole order authority and keep analyses revision-bound
  and non-mutating

Completion check:

- focused stale-ID search is empty on all three files, and every replacement
  retains the correct semantic edge rather than merely changing a label

### Step 3.2 - Resolve pipeline-wide representation choices

Goal: close the architecture choices currently left open by the pipeline.

Primary target:

- `src/backend/bir/pipeline/README.md`

Actions:

- choose one canonical SSA representation and shared `EdgeKey` encoding
- freeze intrinsic namespace, asm-goto result/topology treatment, exception
  edges, and runtime-helper eligibility ownership at the appropriate adjacent
  contracts
- synchronize affected local owner documents in the same packet; do not let
  pipeline become a duplicate semantic owner

Completion check:

- the pipeline has no unresolved acceptance-critical choice and every chosen
  rule points to exactly one local owner

### Step 4 - Repair B1-B2 canonical pass identifiers

Goal: align the first canonical pass pair and comparison dependency with the
root registry.

Review in this order:

1. `src/backend/bir/passes/legalize/README.md`
2. `src/backend/bir/passes/scalar/README.md`
3. `src/backend/bir/analysis/comparison/README.md`

Completion check:

- no stale S-stage identifier remains and B1/B2 retain target-independent,
  transactional Raw-to-Canonical semantics

### Step 5 - Repair B3-B4 canonical pass identifiers

Goal: align CFG/SSA passes and their analysis/publication dependencies with the
root registry.

Review in this order:

1. `src/backend/bir/passes/cfg/README.md`
2. `src/backend/bir/analysis/cfg/README.md`
3. `src/backend/bir/analysis/dominance/README.md`
4. `src/backend/bir/passes/ssa/README.md`
5. `src/backend/bir/analysis/publication/README.md`

Completion check:

- no stale S-stage identifier remains; terminator, edge, SSA, and publication
  rules agree with the choices closed in Step 3.2

### Step 6 - Repair B5-B8 identifiers and Canonical publication

Goal: finish A-F propagation through the remaining canonical passes and the
Canonical verifier boundary.

Review in this order:

1. `src/backend/bir/passes/memory/README.md`
2. `src/backend/bir/passes/aggregate/README.md`
3. `src/backend/bir/passes/intrinsics/README.md`
4. the Raw/Canonical profiles in `src/backend/bir/verify/README.md`

Actions:

- remove stale `S06`-`S08`/`G01` language and bind each pass to B5-B8
- make Raw/Canonical reachability and publication meanings exact
- keep unimplemented source coverage distinct from closed verifier semantics

Completion check:

- no obsolete registry identifier remains on canonical pass or verifier
  surfaces; the Canonical gate accepts exactly the B7 output under B8

### Step 7 - Close target-preparation and verifier-profile boundaries

Goal: resolve the remaining target-preparation/C9 profile choices in the
normative review order.

Review in this order:

1. `src/backend/bir/target_layout/README.md`
2. `src/backend/bir/preparation/README.md`
3. `src/backend/bir/preparation/abi/README.md`
4. `src/backend/bir/preparation/calls/README.md`
5. `src/backend/bir/preparation/variadic/README.md`
6. `src/backend/bir/preparation/address/README.md`
7. `src/backend/bir/preparation/inline_asm/README.md`
8. `src/backend/bir/preparation/runtime_helpers/README.md`
9. `src/backend/bir/regalloc/constraints/README.md`
10. the preparation-facing profiles in `src/backend/bir/verify/README.md`

Actions:

- define one exact C9 binding API and its Canonical revision/target key
- choose and use one meaning for `Canonical` versus `PreparedInput`
- ensure each preparation fact has one producer, consumer, invalidation rule,
  and revision/target binding
- retain C7 table ownership and C9 original-constraint interpretation

Completion check:

- no unresolved preparation/C9/profile question remains, and the review order
  matches the root execution dependency order with inline asm before helpers

### Step 8 - Add and index the complete D2 subordinate contract

Goal: give shared ABI-aware call lowering a full local contract rather than a
root-only owner or abbreviated D1 handoff.

Primary targets:

- `src/backend/bir/passes/call_lowering/README.md` as the dedicated D2 owner
- `src/backend/bir/README.md`
- `src/backend/bir/passes/pseudo_lowering/README.md`
- adjacent C4, D1, D3, and D4 contracts

Actions:

- define D2 admitted operations, exact input/output revision and product keys,
  ABI-rule selection, preservation/invalidation, failure atomicity, legacy
  disposition, and D1/D3/D4 adjacency
- index the new document exactly once in the root and local pass inventory
- remove any claim that the abbreviated D1 handoff is the complete D2 owner

Completion check:

- D2 has one indexed subordinate owner and no required contract is root-only;
  the recalculated Markdown inventory and links are exact

### Step 9.1 - Choose the BIR-owned D5 parallel-copy realization route

Goal: make cyclic and overlapping D5 copies realizable without weakening the
strict E4/MIR contract, and make every product consumed after copy resolution
exact-current for the resolved candidate revision.

Primary targets:

- `src/backend/bir/passes/out_of_ssa/README.md`
- `src/backend/bir/pseudo/README.md`
- the D5 profile in `src/backend/bir/verify/README.md`
- the root D5-to-E4 flow

Actions:

- name the BIR-owned allocation-aware resolution point and its exact position
  before E4
- define how cycles, overlap, scratch homes, and failure are handled
- define whether `ParallelCopy` is an intermediate-only node and the exact
  verifier gate at which it must be absent
- preserve stable identities and transactional publication without claiming
  simultaneous multi-copy is one machine instruction
- choose one coherent publication route for the revision advanced by copy
  resolution: either a named registered owner validates and installs exact-key
  E1 liveness, E2 assignment, E3 spill/reload state, and target-realizability
  products for the new revision, or the stage order/publication contract is
  changed so E4 never treats predecessor-keyed products as current
- for every preserved, rekeyed, or recomputed product, name the invoking owner,
  source and output keys, preservation validator or recomputation rule,
  invalidation behavior, and all-or-nothing rollback boundary
- keep `ConstraintProjectionTransaction` as the sole projected-constraint
  authority; do not claim its preservation record also mints E1/E2/E3 or
  realizability products

Completion check:

- every admitted D5 copy shape has a documented route to directly realizable
  BIR nodes before E4, or fails transactionally before publication
- the resolved revision has exact-current E1, E2, E3 spill, projected
  constraint, and target-realizability products through named owners, and E4
  cannot consume a product keyed only to the initial-D5 or E3-retry revision

### Step 9.2 - Reconcile allocation and strict downstream realization

Goal: synchronize the chosen D5 route with liveness, allocation, spill/reload,
frame-aware realizability, verifier intervals, E4, and MIR consumption.

Review in this order:

1. `src/backend/bir/analysis/liveness/README.md`
2. `src/backend/bir/regalloc/README.md`
3. `src/backend/bir/regalloc/spill_reload/README.md`
4. `src/backend/bir/pseudo/README.md`
5. `src/backend/bir/passes/out_of_ssa/README.md`
6. `src/backend/bir/regalloc/constraints/README.md`
7. `src/backend/bir/allocated/README.md`
8. `src/backend/bir/pipeline/README.md`
9. the E1-E4 profiles and ownership coverage ledgers in
   `src/backend/bir/verify/README.md`
10. root F1-F3 boundary language and the normative MIR boundary

Actions:

- add one BIR-owned post-allocation realizability input and closure that knows
  enough abstract frame-placement bounds to prove direct one-record mapping,
  or restrict the admitted allocated schema so the same proof is complete
- choose and document one explicit BIR-owned post-E3 producer/schema route for
  every required final-frame action. The route must admit an explicit
  one-record representation for frame setup/teardown, stack adjustment,
  callee-save/restore, and any other required frame action; it may use a
  bounded E4 subordinate mutator followed by full exact-current reprojection
  and recomputation, or another coherent BIR-owned route with equivalent
  publication guarantees
- if the route mutates after E3, name its input and output revisions, admitted
  node/schema additions, stable-identity rules, sole invoking owner,
  invalidation behavior, exact-current `ProjectedConstraintSet`, E1, E2, E3,
  frame, and target-realizability recomputation order, verifier interval, and
  all-or-nothing rollback boundary; no predecessor-keyed product may be
  relabeled current
- synchronize the sole C9 `ConstraintProjectionTransaction` owner with the
  selected private-D5/E4-materialized route: D5 contributes its complete copy
  mutation, replacement, and tombstone lineage without publishing a separate
  current projection; E4 invokes the sole projection authority exactly once
  after frame-action materialization
- require the final `ProjectedConstraintKey` to bind the materialized
  `PipelineStageStamp`, `CopyResolutionFingerprint`,
  `FrameActionFingerprint`, and both complete D5 and frame-action mutation,
  replacement, and tombstone summaries; projection remains the first product
  in the atomic E4 six-product closure
- cover E3 `Spill`/`Reload`, D2 outgoing-stack operations, dynamic-frame
  interactions, call/frame accesses, scratch state, and any displacement or
  address-materialization limit that could otherwise require multiple MIR
  records
- do not treat failure-only handling for ordinary spill or call frames as the
  realization route: supported frame-requiring capacity spills and calls must
  have a named BIR-owned path to admitted one-record actions before F1
- require that closure to reject transactionally before E4 when one-record
  mapping cannot be proved; MIR/F1 remains a non-repairing mapper and cannot
  return capacity, spill, copy, frame, or address repair to D4
- reconcile `pipeline/README.md` and `verify/README.md` coverage and legacy
  ledgers so E4 is the exact owner of the private `FrameRealizationPlan` and
  F1 is apply-only; remove every remaining claim that MIR/backend or an
  outside-BIR owner chooses frame layout, offsets, storage plans, or
  prologue/epilogue policy
- define separate verifier intervals for the initial-D5 Pseudo publication and
  the assigned E3-retry/D5-resolved private candidate: the first forbids
  allocation facts, while the latter admits the exact assignments and explicit
  spill state required by copy resolution
- make E4's cumulative Pseudo recheck name the private-candidate interval while
  retaining the same graph, Pseudo-schema, revision, and failure-atomicity
  rules

Completion check:

- all required scratch/copy/spill state is allocated and verified before E4;
  every non-`InlineAsm` node consumed by MIR is directly realizable under the
  chosen strict mapping, and MIR/backend has no repair escape hatch
- every required final-frame action has one explicit admitted post-E3 BIR
  producer/schema route, including ordinary nonzero spill/call frames; the
  route is not merely a transactional failure clause
- stack, call, spill, reload, frame, and scratch nodes each have a documented
  one-record proof before `MirReadyBirView` publication, and the verifier no
  longer both forbids and requires allocation state on the same D5-to-E4
  candidate interval
- the pipeline and verifier ownership ledgers agree that E4 privately owns the
  exact `FrameRealizationPlan` and F1 only applies it
- the C9 projection owner, D5 owner, E4 owner, pseudo schema, verifier, and root
  all agree that private D5 publishes no standalone projection and that E4's
  single post-materialization projection key covers both mutation lineages and
  both fingerprints

### Step 10 - Freeze per-revision constraint projection ownership

Goal: define one unambiguous constraint lineage across every mutating revision.

Review in this order:

1. C9 `regalloc/constraints`
2. D1 pseudo lowering
3. D2 call lowering
4. D4 target legalization
5. D5 out-of-SSA and its chosen copy-resolution point
6. E3 spill/reload mutation
7. E1/E2/E4 consumer and verifier profiles

Actions:

- distinguish the immutable Canonical `BoundConstraintSet` from the projected
  binding product for each later revision
- name one shared projection/preservation authority and, for each mutator,
  the stage responsible for invoking it and publishing the exact output key
- define required fingerprints, invalidation, recomputation, failure
  atomicity, and consumer checks at each revision
- preserve the selected final route: D5 copy resolution stages its mutation
  lineage privately without publishing `ProjectedConstraintSet`; after E4
  frame-action materialization, E4 invokes the sole projection authority once
  for the final materialized revision
- make that final `ProjectedConstraintKey` directly bind the materialized
  stamp, `CopyResolutionFingerprint`, `FrameActionFingerprint`, and complete
  mutation/replacement/tombstone summaries for both D5 copy resolution and E4
  frame-action materialization; the projection then commits atomically with
  the exact-current E1, E2, E3, frame, and target products
- reject stable-ID equality as freshness proof

Completion check:

- every published D1/D2/D4/initial-D5/E3 output has exactly one fresh projected
  product or an explicit proof that no product applies; private D5 copy
  resolution has lineage but no published projection; the sole final E4
  projection covers both D5 and frame-action mutations; E1/E2/E4 never consume
  a Canonical-keyed or predecessor product as though it were current

### Step 11 - Reconcile legacy and core dispositions

Goal: give every retained legacy capability one real owner and remove wrong or
absent authority names.

Primary targets:

- `src/backend/bir/LEGACY_COVERAGE.md`
- the legacy map in `src/backend/bir/core/README.md`
- affected root analysis/allocation registry entries

Actions:

- replace the absent `analysis/alias` owner with the actual accepted owner or
  explicitly reject/defer the capability; do not invent an unindexed alias
  contract merely to satisfy the ledger
- verify the core interval/home/spill/reload wording matches E1-E3 BIR
  ownership and strict MIR non-ownership
- inventory the complete current `src/backend/legacy/` tree and map every file
  or explicitly exhaustive family to exactly one ledger row
- add exact dispositions for the nested `prealloc/stack_layout/**`,
  `prealloc/regalloc/**`, and `prealloc/prepared_printer/**` families rather
  than relying on parent-name shorthand or a core-only evidence row
- correct every address-preparation owner from C5 to C6 in the ledger and core
  map, while preserving C5 exclusively for variadic planning
- recheck every `Accepted` row for exactly one present, indexed owner and every
  Reject/Defer row for an explicit boundary

Completion check:

- no ledger owner is absent or ambiguous, and no core/legacy disposition sends
  ordinary allocation or pressure repair outside BIR
- an exhaustive current legacy-path check finds exactly one ledger disposition
  per path/family, including all nested stack-layout, regalloc, and prepared
  printer files, with no address capability assigned to C5

### Step 12 - Reconcile the root and run the complete documentation proof

Goal: make the root overview describe the repaired subordinate architecture
before independent review.

Actions:

- regenerate the complete Markdown inventory, including the D2 document
- reconcile every root link, A-F position, adjacent profile, analysis/planner
  dependency, verifier gate, projection product, copy-resolution edge, retry
  edge, and implementation-status statement
- prove that every product required after D5 copy resolution is exact-current,
  every admitted stack/call/spill/frame node has a BIR-owned one-record
  realizability proof, every required post-E3 final-frame action has an
  explicit BIR-owned producer/schema route, the pipeline and verifier ledgers
  assign the exact private `FrameRealizationPlan` to E4 and make F1 apply-only,
  the sole C9 projection owner binds the final materialized stamp plus both D5
  and frame-action fingerprints and complete mutation/replacement/tombstone
  summaries without a separate pre-E4 D5 projection, and the two D5 verifier
  intervals are distinct
- run `git diff --check`, local Markdown link/path validation, and focused
  searches for stale IDs, duplicate authority, unresolved acceptance choices,
  copied graphs, late allocation repair, absent legacy paths/owners, stale C5
  address ownership, and revision-key ambiguity
- do not record architecture acceptance in this step

Completion check:

- structural proof is green and the root is a self-contained exact normative
  index of the repaired architecture, with no acceptance marker

### Step 13 - Repeat the independent full architecture review

Goal: obtain an independent judgment on the complete repaired documentation.

Actions:

- review every `src/backend/bir/**/*.md` file against the root index,
  `REVIEW_TEMPLATE.md`, this runbook, and idea 731
- audit all adjacent profiles, open questions, A-F identifiers, D2 ownership,
  D5 realizability, projection keys, and legacy dispositions
- independently recheck the four blockers from
  `review/731_full_architecture_review_repeat.md`: exact-current E1/E2/E3 and
  realizability products after the D5 revision advance; BIR-owned frame-aware
  one-record closure; distinct initial-D5 versus assigned retry/resolved
  verifier intervals; and exhaustive legacy-path ownership with C6 address
  attribution
- independently recheck both blockers from
  `review/731_post_repair_full_architecture_review.md`: an admitted BIR-owned
  post-E3 producer/schema route for every required final-frame action, and
  synchronized pipeline/verifier ledgers in which E4 owns the exact private
  `FrameRealizationPlan` while F1 is apply-only
- independently recheck the blocker from
  `review/731_final_frame_action_architecture_review.md`: the sole C9
  projection owner must describe private D5 lineage with no standalone
  projection, one E4 invocation after frame-action materialization, and a
  final key covering the materialized stamp, `CopyResolutionFingerprint`,
  `FrameActionFingerprint`, and both complete mutation/replacement/tombstone
  summaries
- explicitly re-judge the strict no-late-allocation-repair rule and verify it
  has not been used to conceal an unrealizable post-D5 node
- classify every finding as resolved, intentionally deferred implementation,
  or blocking architecture desynchronization

Completion check:

- an independent reviewer reports no blocking architecture, authority, order,
  profile, revision-key, realizability, or legacy-disposition finding

If the review finds any blocker, Step 14 is forbidden. Return to the earliest
affected repair step, update the runbook if the route itself must change, and
repeat Steps 12-13 after repair.

### Step 14 - Record architecture acceptance

Goal: record acceptance only for the exact independently reviewed document
state.

Primary target:

- `src/backend/bir/README.md`

Actions:

- confirm the Step 13 review covers the current document revisions and has no
  blocker
- add only the explicit architecture-accepted checkpoint; do not make a new
  architecture choice or substantive subordinate/root correction here
- leave idea 731 open and implementation gated pending a separate plan-owner
  decision and implementation runbook

Completion check:

- the acceptance marker identifies the independently reviewed architecture
  checkpoint, the root inventory remains exact, and no implementation is
  authorized by this docs-only runbook

## Final Documentation Proof

- `git diff --check`
- exact Markdown inventory and root-index reconciliation
- local Markdown link/path validation
- no stale `S00`-`S29` or `G01` authority language on repaired surfaces
- no unresolved acceptance-critical core/pipeline/verifier question
- exactly one indexed D2 subordinate owner
- a verified BIR-owned D5 copy-realization route before E4
- exact per-revision constraint projection ownership through E4
- no absent/wrong legacy owner and no MIR/backend allocation-repair route
- independent full architecture review against idea 731

No code build, test result, or narrow structural check substitutes for the
independent architecture review.
