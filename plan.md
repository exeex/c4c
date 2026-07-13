# Idea 731 Accepted BIR Architecture Implementation Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Activated from: the architecture accepted at commit
`edab15ee77b8a0695e43b890c3e4057b1a739f38`, covering independently reviewed
documentation checkpoint `8a7404a265ab24e230dcf4d001d6d1033e8d9736`
Supersedes: the completed BIR architecture documentation repair runbook

## Purpose

Implement idea 731 against the accepted A1-F3 architecture without reopening
stage ownership, weakening exact-revision publication, or turning MIR into a
second allocator.

## Goal

Deliver an end-to-end, verified path from the existing structured inline-asm
source carrier through Canonical BIR, target preparation, pseudo lowering,
shared BIR allocation, immutable MIR-ready publication, strict machine mapping,
and late assembly for the supported RV64, AArch64, and x86 profiles.

## Core Rule

The accepted architecture is the implementation contract. Raw and Canonical
BIR remain target-independent and unallocated. All target-specific expansion,
ordinary allocation, pressure spill/reload, copy resolution, frame-action
materialization, and one-record realizability proof finish in BIR. F1 applies
the exact immutable E4 view and cannot allocate, repair, expand, or synthesize
hidden records.

If implementation proves an accepted contract impossible or internally
incomplete, stop the packet and return to architecture review. Do not silently
change the route in code or weaken a verifier/test expectation.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `src/backend/bir/README.md`
- the local accepted README for every owner touched by a packet
- `src/backend/bir/verify/README.md`
- `src/backend/mir/README.md`
- `src/backend/bir/REVIEW_TEMPLATE.md`
- `src/backend/bir/LEGACY_COVERAGE.md`

## Accepted Starting Point

- Architecture acceptance is recorded for documentation checkpoint
  `8a7404a265ab24e230dcf4d001d6d1033e8d9736` and does not itself prove any
  implementation.
- The structured ordinary SSA input/result transport for non-goto inline asm is
  already implemented. General LIR opcode/module import remains outside this
  idea's bounded route.
- Raw/Canonical core, builder, views, the bounded importer, and part of the
  verifier exist. Most B1-F3 owners are deferred, scaffolded, or build-excluded.
- The exact A1-F3 order, product keys, verifier intervals, failure atomicity,
  finite frame-action schema, and strict F1 boundary are closed decisions.
- Idea 731 remains open until its implementation acceptance criteria and final
  regression/document reconciliation are satisfied.

## Scope

- finish the idea-required original inline-asm semantic-field authority without
  parsing asm or target constraints early
- make the accepted BIR pipeline, analyses, verifiers, and immutable capability
  model executable
- implement target layout, preparation, typed constraint binding/projection,
  pseudo formation, shared call lowering, target legalization, out-of-SSA,
  allocation, spill/reload, E4 publication, and strict MIR consumption
- support the reviewed RV64, AArch64, x86-64, and i686 target-profile families
  through shared BIR algorithms and target data/rules
- integrate opaque inline-asm substitution and parsing only at late assembly
- update local implementation-status documentation only with matching code and
  proof, then reconcile the required final README surfaces

## Non-Goals

- no general LIR-to-BIR lowering for unrelated instruction families merely to
  enlarge test coverage
- no restoration or compilation of `src/backend/legacy/**`, old prealloc/MIR,
  removed `c4c-as`, or deleted `src/backend/bir/mir/**`
- no target interpretation in parser, HIR, LIR import, Raw BIR, or Canonical BIR
- no concrete register names, target opcodes, or frame offsets in BIR nodes
- no target-specific ordinary allocator or MIR/backend pressure-repair path
- no broad object/linker/runtime redesign beyond wiring the accepted path into
  existing F3 facilities
- no guessed GCC/LLVM constraint compatibility and no `VRM1` acceptance
- no testcase-shaped matcher, named-case shortcut, supported-to-unsupported
  downgrade, or expectation rewrite claimed as capability progress

## Working Model

Each step is a milestone and may be split by the supervisor into bounded
executor packets recorded in `todo.md`. A packet owns only its named stage,
immediate prerequisites, tests, build integration, and local contract status.
The supervisor chooses exact proof commands and commit boundaries.

Products are immutable and keyed to the complete accepted revision axes. A
mutation invalidates all declared successors. Cancellation or validation
failure publishes no capability, partial graph, cache entry, or mixed product.
Stable IDs, structural equality, or copied records never establish freshness.

## Execution Rules

1. Execute the ordered steps below. Do not skip a publication/verifier gate to
   reach an end-to-end testcase sooner.
2. Start each code packet with matching before-proof selected by the supervisor;
   run a fresh build plus the delegated narrow subset after the change.
3. Use shared semantic/data-driven rules. Target variation belongs in validated
   tables and registered mappings, not architecture-name branches inside the
   shared allocator or constraint interpreter.
4. Keep every packet failure-atomic and revision-key exact before adding the
   next consumer. Never temporarily publish an invalid capability as scaffolding.
5. Add neighboring same-feature tests for every new supported path. A single
   named inline-asm testcase is never sufficient proof of a semantic stage.
6. Preserve original asm/constraint payload and ordinary SSA identities through
   every stage; typed requirements constrain homes but never collapse values.
7. Update a local README's implementation status only when the owned code and
   proof justify it. Architecture changes require a reviewer checkpoint first.
8. Run a broader matching CTest checkpoint after Steps 3, 6, 10, 12, and 14.
   Use the repo regression guard when the supervisor treats a packet as a
   milestone or the blast radius crosses several buckets.
9. Before final closure, obtain independent review against the source idea and
   accepted architecture, then run the supervisor-selected full regression.
10. Keep idea 731 open if this runbook is retired, blocked, or replaced before
    the durable acceptance criteria are actually proved.

## Ordered Implementation Steps

### Step 1.1 - Establish executable pipeline identity and transaction foundations

Goal: make the accepted stage/revision/product model available to later code
without implementing target semantics early.

Primary targets:

- BIR core IDs, storage, views, results, and verifier capability types
- new pipeline/pass/analysis implementation surfaces matching the accepted
  `pipeline`, `passes`, `analysis/publication`, and `verify` contracts
- build metadata and focused unit tests

Actions:

- inventory existing Raw/Canonical bootstrap APIs and reuse their stable-ID and
  immutable-storage conventions
- implement exact `PipelineStageStamp`, schema/fingerprint axes, typed stage
  results, cancellation, deterministic ID reservation, invalidation records,
  and private transaction/rollback foundations
- expose no target fact, allocation state, or mutable published graph
- add build integration and tests for deterministic keys, stale-product
  rejection, cancellation, rollback, and capability lifetime/ownership

Completion check:

- the foundation builds in the normal tree; focused tests prove exact-key and
  failure-atomic behavior; no later stage is falsely reported implemented

### Step 1.2 - Finish source payload authority and A1-A2 supported-path gates

Goal: complete the idea-required inline-asm semantic carrier and make the
existing bounded import path a sound input to the new pipeline.

Actions:

- keep original asm/constraint text, clobbers, effects, and ordinary structured
  LIR uses/results authoritative and separate from LLVM-compatible rendering
- preserve the existing non-goto structured SSA path; do not expand into
  general LIR import or parse target meaning
- complete A2 Raw verification/publication rules needed by the admitted
  carrier and prove malformed payload/value references publish no `RawBir`
- reconcile the scoped HIR/LIR/BIR tests and local transport documentation

Completion check:

- original payload survives unchanged, renderings are non-authoritative, and
  supported structured input/output/read-write cases reach verified Raw BIR
  with distinct ordinary identities

### Step 2 - Implement B1-B4 canonical foundations

Goal: make legalize, scalar, CFG, and SSA canonicalization executable in the
accepted order.

Actions:

- implement the pass runner and mandatory B1-B4 occurrence sequence
- implement comparison, CFG, dominance, and publication dependencies with
  exact revision invalidation
- implement transactional legalize/scalar/CFG/SSA transforms and their verifier
  postconditions, including explicit Phi SSA and exact `EdgeKey`
- prove target independence and absence of allocation/preparation facts

Completion check:

- B1-B4 run deterministically on admitted Raw modules, each advances the exact
  stamp or rolls back, and focused CFG/SSA/value-integrity tests are green

### Step 3 - Implement B5-B8 and Canonical publication

Goal: publish one exact target-independent `CanonicalBir` through the complete
mandatory B1-B8 pipeline.

Actions:

- implement memory, aggregate, and intrinsic canonicalizers plus required
  memory-effect, provenance, and call-graph analyses
- implement the B8 Canonical verifier/publication gate and cumulative stamp
- reject unimplemented source shapes explicitly without weakening supported
  contracts or fabricating target meaning
- run a broader canonical-pipeline regression checkpoint

Completion check:

- every successful supported module traverses B1-B8 in order and publishes one
  verified Canonical capability; failures leave Raw input unchanged

### Step 4 - Implement C1-C2 target binding and verified layout

Goal: derive exact immutable target layouts without mutating Canonical BIR.

Actions:

- implement `TargetProfile` validation and `VerifiedPreparationInput`
- implement data-driven layouts for reviewed RV64 LP64 variants, AArch64
  AAPCS64, x86-64 SysV, and i686 SysV
- verify capacities, aliases, groups, reserved/eligible sets, ABI eligibility,
  mapping domains, and complete fingerprints
- reject unknown/inconsistent profiles transactionally

Completion check:

- layout tests cover scalar and group capacities, aliases, reserved units,
  profile mismatches, and exact Canonical/target binding for every family

### Step 5 - Implement C3-C8 immutable preparation

Goal: publish the complete target-bound preparation bundle in accepted order.

Actions:

- implement ABI, call, variadic, address, inline-asm-table, and runtime-helper
  planners as immutable products
- keep classification/planning separate from graph mutation and allocation
- enforce exact predecessor fingerprints, single producers, cancellation, and
  all-or-nothing cumulative bundle publication
- cover ordinary calls, hidden carriers, variadic boundaries, addresses,
  reviewed inline-asm vocabulary, and helper eligibility without parsing asm

Completion check:

- C3-C8 success produces one exact `VerifiedPreparationBundle`; stale or mixed
  products and unsupported requirements fail without partial publication

### Step 6 - Implement C9 binding and shared projection

Goal: interpret reviewed source constraints once and maintain exact bindings
across later revisions.

Actions:

- implement `BoundConstraintSet` for `r`, `=r`, `VR`, `VRM2`, `VRM4`, and
  `VRM8`, including read/write, ties, early-clobbers, and clobbers; reject
  `VRM1` and guessed spellings
- implement the sole `ConstraintProjectionTransaction`, replacement/tombstone
  coverage, occurrence fingerprints, invalidation, and rollback
- test identity preservation without SSA merging and exact stale-key rejection
- run a broader preparation/constraint regression checkpoint

Completion check:

- Canonical binding and every tested later projection are complete, immutable,
  exact-revision keyed, and transactionally rejected on ambiguous lineage

### Step 7 - Implement D1-D3 pseudo formation and shared call lowering

Goal: publish verified Pseudo BIR with all generic and ABI call transport
explicit before target legalization or allocation.

Actions:

- implement the closed pseudo schema and generic D1 lowering
- implement shared D2 ABI-aware call lowering from C3/C4 facts for every target
  profile without concrete register/frame spelling
- invoke C9 projection after each mutation and implement the allocation-free D3
  `PseudoPublicationGate`
- test ordinary/runtime-helper calls, hidden sret/byval/variadic carriers,
  caller clobbers, per-call preservation, inline asm, and rollback

Completion check:

- D3 publishes only admitted unallocated pseudos with exact projections; every
  supported call use/definition/fixed requirement is explicit and verifiable

### Step 8 - Implement D4 target pseudo legalization

Goal: make every admitted pre-allocation node directly one-record realizable,
except the explicitly later D5/E4 families.

Actions:

- implement the registered target legalization/expansion chain for all
  supported profiles
- make every introduced value and constraint ordinary BIR state and project the
  exact resulting revision
- fully rerun Pseudo verification and fail closed when no direct route exists
- forbid ABI reclassification, allocation, or hidden MIR expansion

Completion check:

- the D4 gate proves registered direct mappings for the complete supported
  pseudo set and transactionally rejects unrepresentable nodes

### Step 9 - Implement initial D5 out-of-SSA

Goal: destroy Phi semantics while retaining simultaneous copy requirements for
allocation.

Actions:

- implement edge-local `ParallelCopy`/`EdgeCopy`, deterministic scratch
  reservations, critical-edge handling, provenance, and exact projection
- preserve cycle/overlap semantics and stable identities without scheduling
  bundles or inventing MIR temporaries
- implement initial-D5 publication and verifier coverage

Completion check:

- no Phi semantics reach E1; every transfer is edge-complete and cycle-safe;
  allocation-free initial-D5 verification is green

### Step 10 - Implement E1-E3 shared allocation and spill retry

Goal: assign every allocatable identity or represent pressure with explicit
verified BIR spill state.

Actions:

- implement exact-revision liveness/interference including ties, clobbers,
  fixed homes, groups, calls, copies, and non-spillable scratch reservations
- implement one deterministic shared allocator using layout data for all target
  profiles, with finite choice/eviction behavior and typed spill requests
- implement E3 spill slots and directly realizable `Spill`/`Reload` insertion,
  exact projection, full reverification, and bounded E3-to-E1 retry
- reject scratch spilling, recursive spill candidates, stale products,
  non-progress, and ordinary capacity escape to MIR
- run a broader allocation milestone regression

Completion check:

- stable candidates have complete legal abstract homes and spill coverage;
  capacity cases either succeed through BIR-owned retry or fail closed

### Step 11 - Implement D5 resolution and E4 materialization closure

Goal: turn the stable assigned candidate into one immutable, directly
realizable `AllocatedBir` revision.

Actions:

- implement allocation-aware copy resolution with preassigned scratch homes,
  `CopyResolutionFingerprint`, complete lineage, and no standalone publication
- implement deterministic E4 frame draft and the finite one-record frame-action
  family, including callee-save separation and `FrameActionFingerprint`
- invoke final C9 projection after both mutations, then recompute/validate E1,
  E2, E3, frame realization, and target realizability in exact order
- atomically mint `AllocatedBir`, graphless `PreparedBir`, and borrowing
  `MirReadyBirView`; reject every mixed or predecessor product

Completion check:

- no `ParallelCopy`/`CopyScratch`, implicit frame record, unassigned value, or
  unrealizable node reaches the view; failure publishes no revision/product

### Step 12 - Implement E4 verification and capability hardening

Goal: make the accepted public/private verifier intervals and ownership rules
executable and misuse-resistant.

Actions:

- complete `AssignedAllocationCandidateGate` and Allocated/MIR-ready gate
- prove borrowing/lifetime rules, same-revision graph ownership, exact product
  keys, one-record mappings, and graphless readiness capability
- add negative tests for copied graphs, stale/mixed products, hidden frame work,
  late repair requests, forbidden concrete state, and capability misuse
- run a broader end-to-E4 regression checkpoint

Completion check:

- only the exact immutable materialized revision can mint a usable
  `MirReadyBirView`; every forbidden escape is rejected deterministically

### Step 13.1 - Implement strict F1 mapping and RV64 integration

Goal: consume the exact view through an apply-only machine-graph boundary.

Actions:

- implement shared F1 view/key checks and one-record mapping API
- map abstract homes, frame plan, explicit frame nodes, copies, calls, ordinary
  pseudos, and inline-asm operands for reviewed RV64 profiles
- integrate with the current RV64 machine graph without allocation, hidden
  expansion, or pressure/frame repair

Completion check:

- RV64 machine construction succeeds only from the exact view and emits one
  registered record per BIR node; mapping failures leave no machine graph

### Step 13.2 - Implement AArch64 and x86 F1 mappings

Goal: prove that target variation is data/rules over the shared BIR route.

Actions:

- implement reviewed AArch64, x86-64, and i686 mappings against the same F1 API
- cover ABI slots, register groups, calls, spills, frame actions, and inline-asm
  operand substitution without target-specific ordinary allocation
- add cross-target parity and rejection tests

Completion check:

- all supported target families consume the same verified BIR capabilities and
  satisfy the strict apply-only boundary

### Step 14 - Complete F2-F3 verification, late assembly, and integration

Goal: carry the verified machine graph through late inline-asm parsing and the
existing emission path.

Actions:

- implement/repair machine verification for exact mappings and forbidden late
  allocation/expansion
- substitute allocated operands and parse original inline-asm text only in the
  late assembler; preserve invalid-payload late failure behavior
- connect to existing encoding/object/link facilities without broad subsystem
  redesign
- cover supported scalar/group constraints, read/write, ties, early-clobbers,
  clobbers, multi-output identity, spill pressure, calls, and failure cases
- run the broader end-to-end milestone regression

Completion check:

- supported programs reach encoded output through A1-F3; invalid assembly fails
  late; no earlier stage parses asm text or performs late allocation repair

### Step 15 - Reconcile implementation, documentation, and final proof

Goal: decide idea 731 completion from exact implementation evidence.

Actions:

- reconcile implementation status in every touched local owner and finally
  `core`, `lir_to_bir`, `verify`, root BIR, and MIR boundary documentation
- enumerate every intentional deferred item and every implementation/README
  mismatch; accidental desynchronization must be repaired, not relabeled
- independently review the full implementation diff against idea 731, the
  accepted architecture, authority boundaries, and overfit reject signals
- run the supervisor-selected full build/regression guard and confirm legacy
  sources remain excluded
- return to the plan owner for the separate source-idea completion decision;
  do not close solely because this runbook's checklist is exhausted

Completion check:

- independent review has no blocker, full regression is green, documentation
  matches implementation, and every durable acceptance criterion has explicit
  evidence or a clearly authorized separate open initiative

## Final Validation Ladder

1. fresh build for every code packet
2. delegated narrow tests for the owned stage and its immediate boundaries
3. matching broader CTest/regression-guard checkpoints at the named milestones
4. cross-target end-to-end tests for RV64, AArch64, x86-64, and i686
5. exact full regression selected by the supervisor
6. independent final review against idea 731 and the accepted architecture

No expectation rewrite, unsupported downgrade, documentation-only claim, or
single target testcase substitutes for semantic implementation proof.
