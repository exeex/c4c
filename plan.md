# Out-Of-SSA Critical-Edge And Parallel-Copy Deepening

Status: Active
Source Idea: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Activated from: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md

## Purpose

Continue the post-idea-87 `out_of_ssa` route by making critical-edge
handling, parallel-copy ordering, cycle breaking, and copy coalescing policy
publish as explicit target-independent authority.

## Goal

Leave idea 90 with an honest `out_of_ssa` runbook:

- critical-edge handling is published explicitly
- parallel-copy bundles carry enough sequencing and carrier authority for hard
  CFG cases
- downstream consumers can read the published edge/copy facts directly instead
  of inferring them from implementation accidents

## Core Rule

Keep this route target-independent. Do not hide missing `out_of_ssa` contract
semantics behind x86-local copy recovery, testcase-shaped ordering hacks, or
expectation downgrades.

## Read First

- [ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md](/workspaces/c4c/ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md)
- [ideas/closed/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md](/workspaces/c4c/ideas/closed/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md)

## Current Targets

- explicit critical-edge policy in published `out_of_ssa` authority
- stronger `PreparedParallelCopyBundle` semantics for ordering and temporary
  carriers
- clearer copy-coalescing and normalization boundaries at CFG joins and edges
- dumps and tests that prove downstream consumers can trust the richer edge
  copy contract directly

## Non-Goals

- target-specific register-copy spelling or final assembly rendering
- reopening frame/stack/call authority work now closed under ideas 88 and 91
- grouped-register allocation semantics except where grouped occupancy changes
  published edge-copy rules
- reassigning phi elimination ownership away from `out_of_ssa`

## Working Model

- `out_of_ssa` remains the sole owner of phi elimination for the covered cases
- `join_transfers` and `parallel_copy_bundles` should carry final
  target-independent edge-copy authority for harder CFG shapes
- backends and later prealloc consumers should follow the published copy plan
  instead of reconstructing ordering or carrier policy locally

## Execution Rules

- inspect existing `out_of_ssa`, prepared dump, and consumer surfaces first
- keep packets small and tied to one missing authority seam at a time
- prefer semantic copy-order and carrier rules over named-case shortcuts
- use `build -> focused proof -> broader validation only when blast radius
  expands`
- if execution reveals a separate initiative outside edge/copy publication,
  record it as a new open idea instead of stretching this runbook

## Step 1: Audit Remaining Edge-Copy Contract Gaps

Goal: identify the first missing target-independent fact still blocking idea
90 after idea 87.

Primary targets:

- `out_of_ssa` critical-edge and parallel-copy publication surfaces
- prepared dumps and tests that already consume `join_transfers` and
  `parallel_copy_bundles`
- downstream consumers that still infer ordering or carrier policy from raw CFG
  or implementation detail

Actions:

- inspect the current `out_of_ssa` publication path plus nearby proof coverage
- separate already-published phi and baseline join semantics from the remaining
  critical-edge, ordering, and carrier gaps
- choose the smallest honest next packet that repairs the first missing
  contract fact

Completion check:

- the next implementation packet is concrete, target-independent, and tied to
  one owned edge/copy authority gap

## Step 2: Publish Critical-Edge And Bundle Semantics

Goal: make harder CFG edge-copy obligations explicit in published
`out_of_ssa` authority without collapsing ownership, dump visibility, and
follow-on normalization into one oversized packet.

Primary targets:

- critical-edge handling policy
- richer `PreparedParallelCopyBundle` semantics for edge ownership and
  normalization
- dumps and fixtures that expose the new edge-copy facts directly

Completion check:

- covered critical-edge and bundle cases publish enough authority that later
  code does not need to infer missing edge-copy meaning locally

### Step 2.1: Publish Branch-Owned Join Bundle Lookup Authority

Goal: make existing branch-owned join-transfer bundle authority readable
through one published lookup surface instead of downstream rescans.

Primary targets:

- authoritative lookup for branch-owned `true_bundle` / `false_bundle`
- mixed `predecessor_terminator` and `critical_edge` join ownership
- prepare-level proof for the published lookup seam

Actions:

- keep join-transfer ownership anchored to the published edge records
- expose the matching branch-owned bundle through one target-independent
  helper or equivalent publication surface
- prove the helper on both plain branch joins and mixed critical-edge joins

Completion check:

- downstream readers can obtain existing branch-owned bundle authority without
  reconstructing it from raw `parallel_copy_bundles`

### Step 2.2: Publish Remaining Edge-Owned Bundle Authority

Goal: decide and publish the next uncovered edge-owned bundle semantics that
still force consumers to infer meaning locally.

Primary targets:

- non-join or non-lookup-covered edge-copy bundle ownership
- explicit critical-edge publication boundaries for those bundles
- bundle records that identify ownership without consumer-side CFG
  reinterpretation

Actions:

- inspect which edge-owned bundle families are still outside the Step 2.1
  lookup surface
- choose the smallest target-independent publication seam that closes the
  first remaining ownership gap
- publish that seam in `out_of_ssa` data instead of relying on consumer-side
  reconstruction

Completion check:

- the next uncovered edge-owned bundle family publishes direct ownership
  authority through `out_of_ssa`

### Step 2.3: Expose Bundle Authority In Dumps And Fixtures

Goal: make the Step 2 publication seams visible and testable in prepared
output.

Primary targets:

- prepared dumps for the touched edge-copy families
- focused fixtures for critical-edge and bundle ownership facts
- proof that the published output, not implementation detail, carries the
  contract

Actions:

- extend prepared dumps so the Step 2 bundle ownership facts are visible
  directly
- add or update fixtures around the covered edge-copy families
- keep the proof focused on published authority rather than backend-local
  reconstruction

Completion check:

- prepared dumps and fixtures expose the Step 2 edge/bundle authority
  directly enough for later consumers to trust the publication surface

## Step 3: Make Ordering, Cycle Breaking, And Carrier Use Truthful

Goal: ensure hard parallel-copy cases survive through published ordering and
carrier authority instead of implementation accidents.

Primary targets:

- copy-order and cycle-breaking semantics
- temporary carrier and coalescing boundaries
- downstream consumer surfaces for harder CFG copy bundles

Actions:

- repair the missing ordering/cycle-breaking publication for the selected hard
  bundle cases
- keep temporary carriers and coalescing limits explicit in bundle authority
- add focused proof that downstream consumers can follow the published copy
  plan directly

Completion check:

- harder parallel-copy cases publish truthful ordering and carrier authority
- no owned consumer seam still depends on hidden CFG or implementation detail

## Step 4: Prove Consumer Use And Decide Closure

Goal: confirm the published edge/copy authority is consumable and decide
whether idea 90 is complete or needs follow-on work.

Primary targets:

- prepared dumps for the covered edge/copy families
- backend or downstream consumer proof for the touched authority surfaces
- remaining gaps discovered during Steps 1 through 3

Actions:

- run fresh build proof and focused tests for the changed `out_of_ssa` and
  consumer surfaces
- escalate to broader validation if the blast radius extends beyond one narrow
  bucket
- classify leftovers as remaining idea-90 scope or a separate initiative

Completion check:

- the covered critical-edge and parallel-copy authority is published
  truthfully, consumed directly, and any residual work is honestly split
