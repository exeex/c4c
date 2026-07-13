# BIR Architecture Documentation Convergence Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Activated from: the completed structured-transport runbook checkpoint

## Purpose

Converge the complete BIR architecture as an ordered set of reviewed Markdown
contracts before any deferred implementation begins.

## Goal

Make `src/backend/bir/README.md` the authoritative overview and normative
ordered index for every BIR stage/pass and every Markdown contract beneath
`src/backend/bir/`, then complete and review those contracts in that order.

## Core Rule

This runbook is docs-only. Do not implement target preparation, pseudo
lowering, allocation, spill/reload, allocated publication, MIR, or assembly.
Architecture acceptance is a distinct final gate; green documentation checks
or an exhausted runbook do not authorize implementation and do not close idea
731.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `src/backend/bir/README.md`
- `src/backend/bir/pipeline/README.md`
- `src/backend/bir/REVIEW_TEMPLATE.md`
- `src/backend/bir/LEGACY_COVERAGE.md`

## Scope

- all Markdown files matched by `src/backend/bir/**/*.md`, including root-level
  Markdown files
- stage inputs, outputs, invariants, verifier gates, failure behavior,
  revision/target binding, analysis invalidation, and adjacent-stage ownership
- one root-declared total order for stage/pass review
- exact placement of analyses, preparation facts, pseudo lowering, out-of-SSA,
  allocation, spill/reload, and publication gates
- explicit reconciliation of inline-asm constraint ownership and opaque-text
  handling with the shared BIR allocator

## Non-Goals

- no C++ implementation or build-system changes
- no test expectation or fixture changes
- no legacy/prealloc/old-MIR restoration
- no implementation runbook hidden inside this documentation runbook
- no claim that a scaffold is accepted merely because it has an owner path
- no target-specific allocator or normal pressure spill path outside BIR

## Working Model

The root README owns the overview and normative total order. Subordinate
documents own local contracts only. `pipeline/README.md` expands orchestration
within that root order but cannot supersede it. Analyses are reviewed at the
first stage that requires their facts; planners are ordered dependencies, not
mutating IR stages. Reusing a document at multiple verifier gates does not
create duplicate ownership: the document must define each profile explicitly.

The intended top-level flow to freeze is:

```text
LIR import -> Raw verification
-> target-independent canonical pass pipeline -> Canonical verification
-> TargetProfile/layout derivation -> target preparation context/tables
-> regalloc constraint typing/binding
-> pseudo lowering/schema verification -> optional reviewed target pass gate
-> out-of-SSA -> liveness/allocation/spill-reload loop
-> AllocatedBir verification/publication -> MirReadyBirView -> MIR
```

Step 1 must publish the complete detailed order in the root README. Later
steps may refine local details but may not silently reorder the flow; any
necessary order change must update the root and both adjacent contracts in the
same coherent docs packet.

## Execution Rules

1. Execute steps in numeric order. Do not skip ahead because a later document
   already looks more complete.
2. At each step, inventory the named files and compare them with their immediate
   predecessor and successor contracts.
3. Replace `scaffold`/`under-review` claims only when inputs, outputs,
   invariants, failure behavior, ownership, and legacy coverage are concrete.
4. Keep Raw and Canonical BIR target-independent and unallocated.
5. Keep inline-asm operands/results in the ordinary SSA value model; BIR
   constraint logic interprets structured constraints, while asm text stays
   opaque until the assembler.
6. Keep pseudo-home allocation and ordinary pressure spill/reload in shared
   BIR. MIR may map verified homes and lower admitted pseudo instructions, but
   cannot become a second allocator.
7. Use `src/backend/bir/REVIEW_TEMPLATE.md` at every step and update
   `src/backend/bir/LEGACY_COVERAGE.md` whenever an ownership decision changes
   or exposes an unmapped legacy family.
8. For each docs packet, run `git diff --check`, Markdown-link/path checks, and
   focused `rg` checks for stale or duplicate authority language. Code build
   proof is not required for docs-only changes unless the packet changes
   generated/build metadata (which this runbook forbids).
9. Record progress and proof in `todo.md`; do not rewrite this plan for routine
   completion of one step.
10. Implementation remains blocked until Step 14 records explicit architecture
    acceptance after independent review. Any rejected or unresolved contract
    keeps the gate closed.

## Ordered Steps

### Step 1 - Establish the root overview and normative total order

Goal: make `src/backend/bir/README.md` sufficient for a new agent to understand
the entire BIR architecture and the exact order in which every contract is
reviewed.

Primary target:

- `src/backend/bir/README.md`

Actions:

- publish the complete stage/pass flow, including every verifier gate and the
  allocation retry edge
- index every Markdown file under `src/backend/bir/` exactly once as either a
  stage/pass contract, an analysis/planner dependency reviewed at a named
  point, or a cross-cutting audit contract
- distinguish normative stage order from local orchestration detail and from
  non-serial analysis facts
- state the unimplemented/scaffold reality without weakening the target
  architecture
- freeze the ordered checklist consumed by Steps 2-13

Completion check:

- a fresh `rg --files src/backend/bir -g '*.md'` inventory has no unindexed or
  ambiguously placed file
- the root order includes predecessor, output profile, and verifier boundary
  for every mutating stage/pass

### Step 2 - Converge import, core ownership, and Raw publication

Goal: freeze the lossless LIR-to-Raw-BIR boundary and the immutable Raw carrier.

Review in this order:

1. `src/backend/bir/core/README.md`
2. `src/backend/bir/lir_to_bir/README.md`
3. `src/backend/bir/lir_to_bir/memory/README.md`
4. the Raw profile in `src/backend/bir/verify/README.md`

Actions:

- define identity, ownership, transactionality, generic operands/results, and
  opaque inline-asm transport
- separate implemented bootstrap facts from future general LIR import
- make Raw verification the only publication gate into canonicalization

Completion check:

- importer, core, and Raw verifier agree on the same graph, identity model,
  failure behavior, and unallocated target-independent profile

### Step 3 - Converge pipeline/pass/analysis infrastructure

Goal: freeze orchestration and revision-bound derived-fact rules before local
canonical passes are reviewed.

Review in this order:

1. `src/backend/bir/pipeline/README.md`
2. `src/backend/bir/passes/README.md`
3. `src/backend/bir/analysis/README.md`

Actions:

- align pipeline authority beneath the root total order
- separate the target-independent canonical pass framework from later
  target-aware pseudo/allocated phases
- freeze transactional stage publication, stable-ID preservation, analysis
  invalidation, and verifier-on-commit behavior

Completion check:

- infrastructure documents cannot reorder stages, leak target facts backward,
  or publish stale analysis results

### Step 4 - Converge canonical passes P01-P02 and scalar analysis

Goal: freeze the first two canonical transformations.

Review in this order:

1. `src/backend/bir/passes/legalize/README.md` (P01)
2. `src/backend/bir/passes/scalar/README.md` (P02)
3. `src/backend/bir/analysis/comparison/README.md`

Completion check:

- legalize and scalar have closed input/output profiles, reject unsupported
  semantics explicitly, and do not perform target lowering or allocation

### Step 5 - Converge canonical passes P03-P04 and CFG/SSA analyses

Goal: freeze structural CFG and SSA ownership, including block decomposition.

Review in this order:

1. `src/backend/bir/passes/cfg/README.md` (P03)
2. `src/backend/bir/analysis/cfg/README.md`
3. `src/backend/bir/analysis/dominance/README.md`
4. `src/backend/bir/passes/ssa/README.md` (P04)
5. `src/backend/bir/analysis/publication/README.md`

Completion check:

- terminators are the sole persistent edge authority; split/merge and phi or
  block-argument repair are representable; out-of-SSA is explicitly deferred
  to its later BIR stage rather than MIR

### Step 6 - Converge canonical passes P05-P07 and semantic analyses

Goal: finish the target-independent CanonicalBir transformation sequence.

Review in this order:

1. `src/backend/bir/passes/memory/README.md` (P05)
2. `src/backend/bir/analysis/memory_effects/README.md`
3. `src/backend/bir/analysis/provenance/README.md`
4. `src/backend/bir/passes/aggregate/README.md` (P06)
5. `src/backend/bir/passes/intrinsics/README.md` (P07)
6. `src/backend/bir/analysis/call_graph/README.md`
7. the Canonical profile in `src/backend/bir/verify/README.md`

Completion check:

- CanonicalBir verification accepts the exact P07 output, all semantic facts
  remain target-independent, and inline asm remains one opaque semantic node

### Step 7 - Converge target layout and ordered preparation dependencies

Goal: freeze the target-aware facts that may be derived from verified
CanonicalBir plus `TargetProfile`, without mutating CanonicalBir.

Review in this order:

1. `src/backend/bir/target_layout/README.md`
2. `src/backend/bir/preparation/README.md`
3. `src/backend/bir/preparation/abi/README.md`
4. `src/backend/bir/preparation/calls/README.md`
5. `src/backend/bir/preparation/variadic/README.md`
6. `src/backend/bir/preparation/address/README.md`
7. `src/backend/bir/preparation/runtime_helpers/README.md`
8. `src/backend/bir/preparation/inline_asm/README.md`
9. `src/backend/bir/regalloc/constraints/README.md`

Actions:

- define explicit planner dependency order and revision/profile binding
- assign target vocabulary and eligibility-table ownership to preparation and
  actual parsing/typing/binding of `=r`, `r`, `VR`, `VRM2`, ties, and clobbers
  to BIR allocation constraints
- forbid duplicate constraint owners and MIR allocation facts

Completion check:

- every preparation fact has one producer, one consumer contract, and exact
  revision/target binding; no planner writes allocation into CanonicalBir

### Step 8 - Converge pseudo lowering, schema, and pseudo verification

Goal: freeze the new immutable pseudo-BIR revision admitted to allocation.

Review in this order:

1. `src/backend/bir/passes/pseudo_lowering/README.md`
2. `src/backend/bir/pseudo/README.md`
3. the pseudo profile in `src/backend/bir/verify/README.md`
4. `src/backend/bir/passes/target/README.md`

Actions:

- define the closed admitted pseudo instruction table and explicit lowering
  failure behavior
- preserve IDs for unchanged entities while publishing a new revision
- place optional target-specific pseudo passes behind explicit invalidation and
  reverification gates; keep them deferred and unable to hide allocation

Completion check:

- the pseudo verifier rejects semantic leftovers, target opcodes, concrete
  registers, and malformed inline-asm structure before allocation

### Step 9 - Converge out-of-SSA

Goal: freeze BIR-owned phi/block-argument destruction before allocation.

Review in this order:

1. `src/backend/bir/passes/out_of_ssa/README.md`
2. the affected CFG, SSA, and pseudo verifier profiles

Completion check:

- edge copies/parallel copies, critical-edge handling, stable identity, and
  reverification are explicit; MIR owns none of normal out-of-SSA

### Step 10 - Converge shared liveness and allocation

Goal: freeze one target-shared BIR allocation authority.

Review in this order:

1. `src/backend/bir/analysis/liveness/README.md`
2. `src/backend/bir/regalloc/README.md`
3. cross-check the already accepted
   `src/backend/bir/regalloc/constraints/README.md` consumption profile
4. `src/backend/bir/regalloc/spill_reload/README.md`

Actions:

- distinguish reusable revision-bound liveness facts from allocator policy
- define interference, groups, ties, clobbers, call boundaries, eviction,
  finite pseudo-home assignment, spill-slot identity, and explicit
  Spill/Reload placement
- document the liveness/allocation/spill rewrite/reverification retry loop and
  its termination/failure conditions

Completion check:

- RV64, AArch64, and x86 differ only through reviewed layout data/rules; every
  allocatable value is assigned or represented by verified explicit spill
  state before publication

### Step 11 - Converge AllocatedBir and MIR-ready publication

Goal: freeze the final BIR stage token and its clean read-only downstream view.

Review in this order:

1. `src/backend/bir/allocated/README.md`
2. the Allocated profile in `src/backend/bir/verify/README.md`
3. the `PreparedBir`/`MirReadyBirView` boundary references in preparation,
   pipeline, and root documents

Completion check:

- `AllocatedBir` and `MirReadyBirView` refer to the same immutable revision;
  all homes/spill state and target bindings verify; no duplicate instruction
  graph or ordinary MIR allocation escape hatch remains

### Step 12 - Converge cross-cutting observation and quarantine contracts

Goal: ensure non-stage helpers cannot become hidden semantic authorities.

Review in this order:

1. `src/backend/bir/diagnostics/README.md`
2. `src/backend/bir/compatibility/README.md`
3. `src/backend/bir/LEGACY_COVERAGE.md`
4. `src/backend/bir/REVIEW_TEMPLATE.md`

Completion check:

- diagnostics are read-only; compatibility is quarantined from decisions;
  every retained legacy capability has an accepted owner or an explicit
  disposition; the review template tests the complete new boundary

### Step 13 - Perform full cross-document architecture review

Goal: prove the subordinate documents form one coherent architecture.

Actions:

- inventory every `src/backend/bir/**/*.md` file and compare it with the Step 1
  index
- audit all adjacent stage input/output profiles and all repeated verifier
  profiles
- search for stale claims that allocation, out-of-SSA, physical intervals, or
  ordinary spilling belong to MIR
- search for duplicate inline-asm constraint ownership, target facts in
  Raw/Canonical BIR, concrete target registers/opcodes in pseudo BIR, copied
  PreparedBir graphs, and undocumented pass-order authority
- classify every mismatch as resolved, intentional deferred scope, or blocking
  architecture desynchronization; deferred labels cannot conceal conflicting
  contracts
- obtain an independent architecture review against idea 731

Completion check:

- review reports no blocking authority/order/profile contradiction and every
  Markdown file is accounted for; unresolved findings return execution to the
  earliest affected step

### Step 14 - Reconcile the root overview and record architecture acceptance

Goal: make the final root README describe the accepted subordinate contracts,
not the initial plan for them.

Primary target:

- `src/backend/bir/README.md`

Actions:

- regenerate the Markdown inventory and reconcile every link, stage/pass
  position, analysis/planner dependency, verifier gate, retry edge, and status
  against the final subordinate contracts
- state any intentional deferred implementation scope separately from
  documentation completeness
- record explicit architecture acceptance only if Step 13 has no blocker and
  every subordinate contract required by the index is accepted
- leave idea 731 open and require a later plan-owner decision for a separate
  implementation runbook

Completion check:

- the root README is a self-contained overview and exact normative index of the
  accepted architecture; inventory and cross-reference checks are green; an
  independent reviewer accepts the docs-only architecture checkpoint

## Final Documentation Proof

- `git diff --check`
- exact Markdown inventory reconciliation against the root index
- local Markdown link/path validation
- focused stale-authority searches described in Steps 7, 9, 10, 11, and 13
- independent review against
  `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`

No code build or test result may substitute for this architecture review.
