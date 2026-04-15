# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Activated from: fresh activation on 2026-04-15 after closing the semantic call/runtime boundary runbook

## Purpose

Rebuild `prepare` into the real post-lowering owner so semantic BIR stays
target-neutral and targets consume concrete prepared-phase artifacts instead of
reconstructing legality ad hoc.

## Goal

Make `prepare` own target legality and real prepared-BIR phase outputs for
stack layout, liveness, and register allocation.

## Core Rule

Do not recover backend progress by pushing legality back into semantic
lowering or by letting targets keep reconstructing prepare-owned state.

## Read First

- `ideas/open/48_prepare_pipeline_rebuild.md`
- `src/backend/prepare/legalize.cpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/backend.cpp`
- `src/backend/prepare/stack_layout.cpp`
- `src/backend/prepare/liveness.cpp`
- `src/backend/prepare/regalloc.cpp`
- `ref/claudes-c-compiler/src/backend/stack_layout/mod.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/analysis.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/slot_assignment.rs`
- `ref/claudes-c-compiler/src/backend/liveness.rs`
- `ref/claudes-c-compiler/src/backend/regalloc.rs`

## Current Targets

- `src/backend/prepare/legalize.cpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/backend.cpp`
- `src/backend/prepare/stack_layout.cpp`
- `src/backend/prepare/liveness.cpp`
- `src/backend/prepare/regalloc.cpp`

## Activation Checkpoint

- semantic BIR lowering now covers the call/runtime boundary work needed for
  the shared route to move forward without reopening raw-LIR fallback
- the next dependency in the reboot sequence is to make `prepare` the owner of
  legality and prepared-phase artifacts before target ingestion work
- `ideas/open/49_prepared_bir_target_ingestion.md` depends on this initiative,
  so target reconnect work should stay parked until this runbook establishes a
  real prepared-BIR contract

## Non-Goals

- do not let targets rebuild legality independently of `prepare`
- do not treat current `prepare/*.cpp` placeholders as fixed local designs
- do not route around missing prepare work by reopening direct/raw LIR paths
- do not expand into target-specific ingestion work that belongs to idea 49

## Working Model

- semantic BIR should stay as the target-neutral lowering contract
- `prepare` should transform semantic BIR into a prepared form that encodes
  legality decisions and concrete phase outputs needed by targets
- stack layout, liveness, and regalloc should become explicit phase owners
  with inspectable artifacts instead of notes, shells, or target-side guesses
- the backend driver should make the shared route through `prepare` visible
  before idea 49 reconnects target codegen to prepared BIR only

## Ordered Steps

### 1. Re-establish `prepare` as the shared route owner

Goal: make the backend driver and prepare entrypoint clearly own the
semantic-BIR to prepared-BIR transition.

Primary target:

- `src/backend/prepare/prepare.cpp`
- `src/backend/backend.cpp`

Concrete actions:

- inspect the current backend route and name the exact places where `prepare`
  is still a shell or advisory layer
- define the prepared-phase entry contract in `prepare.cpp` instead of leaving
  ownership implicit in target code paths
- make the shared backend route through `prepare` explicit without yet
  reconnecting target ingestion work that belongs to idea 49
- keep proof framed as `build -> narrow backend subset`, with broader
  validation once shared-route changes land

Completion check:

- another agent can point to one clear prepare entrypoint for shared backend
  routing
- the active route no longer relies on hand-waved prepare ownership
- the first implementation slice can be stated against concrete prepare-owned
  files without drifting into target ingestion

### 2. Move legality into `prepare`

Goal: establish target-legality ownership in the prepare layer.

Primary target:

- `src/backend/prepare/legalize.cpp`
- `src/backend/prepare/prepare.cpp`

Concrete actions:

- decide which semantic-BIR value forms must be legalized before target
  codegen, including `i1` and other target-facing representation constraints
- encode those decisions in prepare-owned helpers rather than target-specific
  workaround paths
- define prepared-BIR invariants that later phases and targets can consume as
  contract, not inference
- keep unsupported behavior named by prepared-phase capability rather than by
  testcase history

Completion check:

- legality decisions have a clear owner under `prepare`
- prepared-BIR invariants are explicit enough for later phase work
- targets no longer need to be treated as the default legality owner in the
  shared route

### 3. Rebuild stack layout as a real prepared phase

Goal: turn stack layout from scaffold into a phase with concrete outputs.

Primary target:

- `src/backend/prepare/stack_layout.cpp`
- reference stack-layout files under `ref/claudes-c-compiler/src/backend/stack_layout/`

Concrete actions:

- define the stack-object and frame-layout inputs expected from prepared BIR
- rebuild slot-assignment and frame-layout logic around prepared-phase data
  rather than placeholder notes
- keep phase outputs explicit enough for liveness and regalloc to consume
- prove progress with nearby same-shape backend cases instead of one narrow
  testcase family

Completion check:

- stack-layout code produces real prepared artifacts rather than placeholders
- downstream phase requirements are visible in the stack-layout contract
- proof shows shared prepared-phase behavior, not a single testcase shortcut

### 4. Rebuild liveness as the prepared-BIR analysis owner

Goal: make liveness publish the prepared analysis facts that regalloc should
consume instead of recomputing them ad hoc.

Primary target:

- `src/backend/prepare/liveness.cpp`
- `src/backend/prepare/prepare.hpp`
- `ref/claudes-c-compiler/src/backend/liveness.rs`

Concrete actions:

- define the prepared liveness artifact around explicit analysis ownership,
  including instruction-order access windows, call-boundary crossing cues, and
  other prepared facts that naturally belong to liveness instead of regalloc
- grow `PreparedLiveness*` data so downstream phases can consume concrete
  analysis output rather than only `value_storage` vs
  `address_exposed_storage` classification
- keep the prepared liveness contract target-neutral and derived from semantic
  BIR plus stack-layout/legalize outputs, not from target register names or
  testcase-shaped heuristics
- use `ref/claudes-c-compiler/src/backend/liveness.rs` for responsibility
  boundaries and dependency order even if the prepared-BIR data model differs
- prove progress with nearby same-shape backend cases and a broader backend
  checkpoint before treating liveness as a stable phase owner

Completion check:

- liveness emits real prepared-phase analysis artifacts, not only coarse object
  classification
- the repo has one clear answer for where access-window and call-crossing facts
  come from on the prepared route
- regalloc can be restated as a consumer of prepared liveness facts instead of
  as a second analysis owner

### 5. Rebuild register allocation as a consumer of prepared liveness

Goal: make regalloc consume prepared liveness/stack-layout/legalize artifacts
instead of rebuilding their facts inside regalloc-local scaffolding.

Primary target:

- `src/backend/prepare/regalloc.cpp`
- `src/backend/prepare/prepare.hpp`
- `ref/claudes-c-compiler/src/backend/regalloc.rs`

Concrete actions:

- move regalloc inputs and sequencing onto prepared liveness artifacts and
  prepared stack objects rather than direct BIR rescans for access summaries
- keep regalloc focused on allocation staging, reservation/contention,
  binding/handoff, and downstream prepared contract projection
- remove or retire regalloc-local logic whose only purpose is to reconstruct
  liveness-owned facts that should already be present in prepared artifacts
- use `ref/claudes-c-compiler/src/backend/regalloc.rs` for responsibility
  boundaries so `liveness -> regalloc` remains the dependency direction on the
  shared prepared route
- checkpoint broader validation before treating the reconstructed regalloc
  contract as stable

Completion check:

- regalloc consumes prepared liveness facts rather than owning a second shadow
  analysis path
- liveness and regalloc contracts line up with stack layout and legality
  ownership
- the backend route is meaningfully closer to prepared-BIR-only ingestion

### 6. Tighten the prepared-route contract before target reconnection

Goal: leave a stable prepare-owned contract for idea 49.

Primary target:

- `src/backend/prepare/prepare.cpp`
- `src/backend/backend.cpp`
- `src/backend/prepare/legalize.cpp`
- `src/backend/prepare/stack_layout.cpp`
- `src/backend/prepare/liveness.cpp`
- `src/backend/prepare/regalloc.cpp`

Concrete actions:

- replace placeholder or note-only route language with concrete prepare-owned
  contract wording
- verify the backend driver clearly routes semantic BIR through prepare before
  target codegen
- capture any remaining target-ingestion-only work as follow-on scope for idea
  49 instead of silently extending this runbook
- require a broader backend checkpoint before treating this runbook as
  exhausted or complete

Completion check:

- the repo has a stable prepare-owned contract another agent can pick up
- remaining work is clearly separated into prepare-owned vs target-ingestion
  scope
- idea 49 can activate against an honest prepared-BIR baseline
