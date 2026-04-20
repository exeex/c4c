# Complete Phi Legalization And Parallel-Copy Resolution

Status: Active
Source Idea: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Activated from: ideas/closed/62_prealloc_cfg_generalization_and_authoritative_control_flow.md

## Purpose

Turn idea 63 into an execution runbook that replaces pattern-first phi
materialization and slot-backed fallback with a general, typed phi destruction
route built on the authoritative control-flow substrate from ideas 64 and 62.

## Goal

Publish a general prepared phi-transfer contract so legalization, regalloc, and
later consumers can preserve SSA edge semantics without guessing from select
shapes, slot traffic, or surviving IR topology.

## Core Rule

Do not treat one more phi testcase shape as success. New work must generalize
phi destruction and parallel-copy resolution rather than growing named-case
materializers.

## Read First

- `ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md`
- `ideas/closed/62_prealloc_cfg_generalization_and_authoritative_control_flow.md`
- `ideas/closed/64_shared_text_identity_and_semantic_name_table_refactor.md`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`

## Scope

- replace pattern-first phi recovery with a deliberate edge-transfer model
- make prepared phi-transfer data authoritative on typed semantic ids
- resolve parallel-copy and phi-cycle hazards without slot-only fallback being
  the correctness path
- migrate downstream consumers to read authoritative phi-transfer data instead
  of reconstructing phi moves from residual IR shape

## Non-Goals

- new testcase-shaped phi materializers
- unrelated branch/join ownership rewrites already handled by idea 62
- generic scalar instruction selection
- broad x86 emitter rewrites unrelated to consuming prepared phi transfers

## Working Model

- prepare owns authoritative phi incoming-edge analysis per function
- prepared data publishes per-edge transfer facts and any canonical
  parallel-copy grouping needed to preserve move correctness
- critical-edge handling is explicit when the legality model requires it
- regalloc and downstream consumers consume that prepared contract instead of
  reconstructing transfer meaning from select materialization or slot traffic

## Execution Rules

- keep phi correctness in shared prepare, not in x86-local matcher growth
- preserve `FunctionNameId` and `BlockLabelId` as the public identity boundary
- prefer explicit transfer planning plus copy-resolution semantics over
  opportunistic slot stores and loads
- update `todo.md`, not this file, for routine packet progress
- require `build -> narrow proof` for every accepted code slice
- broaden validation when a slice changes shared prepare plus multiple consumer
  families

## Step 1: Inventory The Current Phi Legalization Route

Goal: map the current phi materialization, slot-backed fallback, and regalloc
handoff so the route stays focused on general transfer semantics instead of ad
hoc testcase growth.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`

Actions:

- identify where phi handling still depends on select/funnel recovery or local
  slot-backed fallback
- list the current transfer facts, if any, that later consumers already read
- note the consumer surfaces that would still need to rediscover phi movement
  from surviving IR shape

Completion check:

- the route has a concrete inventory of pattern-first phi paths to replace,
  transfer facts to preserve, and downstream handoff surfaces that must migrate

## Step 2: Build An Authoritative Prepared Phi-Transfer Model

Goal: make shared prepare own per-edge phi transfer semantics as a stable
consumer contract.

### Step 2.1: Publish Typed Phi Edge-Transfer Facts

Goal: expose prepared phi incoming-edge ownership on semantic ids instead of
burying it inside materialized select paths or slot-backed fallback.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`

Actions:

- add or reshape prepared structures so phi incoming transfers are represented
  explicitly per predecessor/successor edge
- keep the prepared surface consumer-oriented rather than leaking
  bootstrap-shaped local recovery details
- preserve typed identity boundaries on functions, blocks, and transferred
  values

Completion check:

- prepared functions publish consumer-readable phi transfer facts without
  requiring downstream code to infer phi movement from CFG or slot traffic

### Step 2.2: Make Parallel-Copy Resolution Authoritative

Goal: represent move ordering and cycle-breaking semantics explicitly so
multi-move phi edges do not depend on incidental lowering order.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- any extracted shared copy-planning helpers needed to keep the algorithm
  coherent

Actions:

- build a canonical parallel-copy or equivalent transfer-resolution model for
  phi edges with multiple incoming moves
- handle cycles and clobber hazards explicitly instead of leaning on slot-only
  fallback as the correctness path
- keep the representation aligned with the typed prepared transfer contract from
  Step 2.1

Completion check:

- shared prepare can publish stable phi transfer resolution for multi-move and
  cyclic cases without testcase-shaped special cases

### Step 2.3: Handle Critical Edges Only Where The Legality Model Requires It

Goal: make edge splitting or equivalent legality handling explicit when phi
transfer execution cannot be represented faithfully on the existing CFG edge.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- any extracted shared helpers needed for edge splitting or transfer placement

Actions:

- identify when the chosen phi-transfer model requires critical-edge handling
- implement the narrow legality mechanism needed to preserve transfer truth
  without widening into unrelated CFG rewrites
- keep the public prepared contract focused on truthful transfer semantics, not
  on incidental temporary lowering details

Completion check:

- the legality path for phi transfer execution is explicit and no remaining
  supported case relies on hidden CFG-shape assumptions

## Step 3: Migrate Consumers To The Authoritative Phi-Transfer Contract

Goal: move regalloc and downstream lowering onto prepared phi-transfer facts so
they stop reconstructing move meaning from residual IR shape.

Primary targets:

- `src/backend/prealloc/regalloc.cpp`
- downstream consumer paths that currently rely on select materialization,
  slot-only fallback, or implicit phi move ordering

Actions:

- consume prepared phi-transfer and copy-resolution data directly in regalloc or
  adjacent downstream handoff code
- remove or fence off local phi reconstruction that becomes redundant once the
  shared contract exists
- keep consumer changes limited to the surfaces needed to consume the new
  prepared facts

Completion check:

- downstream ownership of phi movement reads prepared transfer data directly
  enough that phi correctness no longer depends on select-shaped recovery or
  slot-backed fallback

### Step 3.1: Move Regalloc And Edge-Move Consumers To Prepared Transfers

Goal: make the immediate prepare-to-regalloc and move-resolution handoff
contract authoritative for phi edges.

Primary targets:

- `src/backend/prealloc/regalloc.cpp`
- any adjacent shared transfer-resolution helpers required by regalloc

Actions:

- replace local assumptions about phi move ordering with prepared transfer and
  copy-resolution reads
- prove that single-edge, multi-move, and cyclic transfer cases use the shared
  contract instead of incidental IR order
- keep this step focused on the prepare/regalloc boundary before widening to
  broader target consumers

Completion check:

- regalloc-facing phi move handling no longer needs to rediscover transfer
  meaning from legalized IR shape

### Step 3.2: Keep Downstream Phi Consumers Contract-Strict

Goal: close the remaining consumer paths whose correctness depends on
authoritative prepared phi-transfer data after regalloc adopts the contract.

Primary targets:

- the narrowest downstream consumer families that still observe phi transfer
  semantics after Step 3.1
- focused proof surfaces under `tests/backend/`

Actions:

- add or maintain proof that prepared phi-transfer drift is caught or redirected
  according to the shared contract rather than local fallback recovery
- avoid reopening unrelated branch/join or generic instruction-selection work
- stop once no consumer family still needs bespoke phi movement interpretation

#### Step 3.2.1: Finish Bounded Compare-Join Immediate-Op Consumer Coverage

Goal: finish the remaining joined-branch immediate-op compare-join families in
`backend_x86_handoff_boundary` that still need contract-strict proof beyond
passthrough ownership and render-contract checks.

Primary targets:

- `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`
- the narrow compare-join immediate-op fixture families already covered by the
  authoritative prepared-control-flow handoff

Actions:

- extend one bounded immediate-op family at a time with branch-plan,
  non-compare-entry rejection, and prepared branch condition/record checks
- keep default-carrier and forced-`EdgeStoreSlot` proof paths aligned on the
  same authoritative prepared compare-join contract
- stop once the remaining immediate-op families in this test surface no longer
  rely on weaker passthrough-only proof

Completion check:

- the remaining immediate-op compare-join families under
  `backend_x86_handoff_boundary` prove contract-strict ownership without adding
  emitter-local fallbacks or testcase-shaped route changes

#### Step 3.2.2: Generalize Contract Checks Only Where Family Reuse Demands It

Goal: keep compare-join helper coverage reusable across immediate-op families
without accidentally widening the route into emitter rewrites or test-only
special cases.

Primary targets:

- `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`
- any narrow shared helper assertions in the same test surface that are too
  family-specific for the remaining immediate-op coverage

Actions:

- generalize helper expectations only when the remaining compare-join families
  cannot be covered honestly with the current helper surface
- preserve the authoritative prepared-control-flow contract as the checked
  behavior rather than introducing family-specific escape hatches
- keep helper changes limited to the minimum needed for contract-strict proof

Completion check:

- the immediate-op compare-join proof surface is reusable enough to cover the
  remaining families without duplicating testcase-shaped assertions

#### Step 3.2.3: Re-evaluate Remaining Downstream Consumers After The Immediate-Op Batch

Goal: decide which downstream consumer family still needs Step 3.2 attention
after the bounded immediate-op compare-join batch is complete.

Primary targets:

- `todo.md`
- the narrowest downstream consumer families still named by active
  `tests/backend/` proof once Step 3.2.1 is exhausted

Actions:

- inventory which downstream consumers still observe phi-transfer semantics
  after the immediate-op compare-join batch is complete
- decide whether the next packet stays in the current handoff-boundary surface
  or moves to a different narrow consumer family
- capture that next bounded route in `todo.md` without widening `plan.md`
  beyond the active source idea

Completion check:

- the next Step 3.2 packet after the immediate-op batch is explicitly routed to
  one bounded downstream consumer family instead of continuing as an unlabeled
  catch-all step

Completion check:

- the remaining downstream phi consumers stay aligned with authoritative
  prepared transfer semantics without testcase-shaped recovery

#### Step 3.2.4: Inventory Carrier-Kind Consumers Outside The Joined-Branch Surface

Goal: decide whether any downstream consumer outside the exhausted
`backend_x86_handoff_boundary` joined-branch compare-join families still needs
Step 3.2 attention through the transitional carrier-kind compatibility surface.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- the narrowest remaining proof surface, if any, under `tests/backend/`

Actions:

- inventory non-joined-branch consumers that still branch on
  `effective_prepared_join_transfer_carrier_kind(...)` or equivalent prepared
  carrier-kind compatibility logic
- decide whether one bounded Step 3.2 packet remains outside the current
  joined-branch test surface or whether this downstream consumer route is ready
  to close or re-scope
- capture that next bounded route in `todo.md` without extending the exhausted
  joined-branch test chain

Completion check:

- Step 3.2 is either routed to one specific remaining consumer family outside
  the joined-branch handoff-boundary surface or explicitly ready for
  closure/re-scope without another test-only packet in the same file

## Step 4: Validate The Phi Legalization Route

Goal: prove the general phi-transfer route without bundling unrelated backend
rewrites.

Actions:

- require a fresh `cmake --build --preset default` for each accepted slice
- use a backend-focused proving subset for routine packets
- broaden validation when a slice changes shared prepare plus regalloc and
  multiple downstream consumer families or when the route reaches a
  closure-quality checkpoint

Completion check:

- accepted slices build cleanly and the proving logs show stable phi-transfer
  behavior on the chosen validation scope
