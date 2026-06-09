# Current-block Join Fact and Routing Split

Status: Active
Source Idea: ideas/open/145_current_block_join_fact_routing_split.md

## Purpose

Split reusable current-block join parallel-copy facts from target-facing
AArch64 instruction routing convenience.

## Goal

Make reusable join-copy facts available through shared prealloc ownership while
keeping AArch64 routing booleans and instruction dispatch choices target-local.

## Core Rule

Do not rename the residual prepared lookup surface without separating the
semantic fact owner from target-specific routing convenience.

## Read First

- `ideas/open/145_current_block_join_fact_routing_split.md`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/control_flow.hpp`
- AArch64 dispatch files that currently consume current-block join parallel-copy
  routing helpers
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

## Scope

- Separate current-block join source status/fact naming from AArch64
  instruction routing naming.
- Move reusable join parallel-copy source facts toward
  `src/backend/prealloc/publication_plans.hpp/.cpp` and, where needed,
  `src/backend/prealloc/control_flow.hpp`.
- Localize or separately name target-facing instruction routing convenience in
  AArch64 dispatch code.
- Split value-home/out-of-SSA helper predicates into reusable fact predicates
  and routing use.

## Non-Goals

- Do not move AArch64 register spelling, scratch policy, hazard policy, or final
  instruction emission into shared prealloc code.
- Do not delete reusable join-copy source facts.
- Do not reopen edge-publication facade splitting except for the explicit
  current-block join-copy residual surface.
- Do not fold unrelated return-chain or stack-source publication work into this
  route.
- Do not downgrade tests or weaken expected contracts to claim progress.

## Working Model

- Shared prealloc code should answer reusable semantic questions about
  current-block join parallel-copy sources.
- AArch64 code may still combine those facts into local routing decisions for
  target instruction dispatch.
- RISC-V and x86 reusable fact needs must remain preserved even when the
  immediate consumers are AArch64-oriented.

## Execution Rules

- Keep each implementation step behavior-preserving unless the source idea
  explicitly requires a semantic boundary correction.
- Prefer owner names that describe semantic facts, not dispatch destinations.
- If a helper is useful outside AArch64 routing, place or name it as a shared
  fact predicate before adapting target-local callers.
- If shared publication/control-flow facts change, escalate validation to full
  CTest after the backend subset passes.
- Treat a patch that only proves one named dispatch case while nearby join-copy
  cases remain unexamined as insufficient.

## Steps

### Step 1: Inspect Current Join-copy Fact and Routing Uses

Goal: identify the exact declarations, definitions, and consumers that mix
current-block join-copy facts with AArch64 routing convenience.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- AArch64 dispatch files naming current-block join parallel-copy routing or
  related value-home/out-of-SSA predicates

Actions:

- Locate `PreparedCurrentBlockJoinParallelCopyInstructionRouting` and related
  helper predicates.
- Classify each declaration as either reusable source fact or target-local
  routing convenience.
- Identify every consumer that must be updated when the owner names split.
- Record any ambiguous boundary in `todo.md` before editing code.

Completion check:

- The executor can state which symbols move to shared prealloc ownership, which
  symbols remain target-local, and which files need updates.

### Step 2: Move Reusable Join-copy Source Facts

Goal: place reusable current-block join parallel-copy source facts under shared
prealloc ownership.

Primary targets:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/control_flow.hpp`, only if the fact boundary clearly
  belongs there
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:

- Rename or move reusable join-copy source status/fact declarations away from
  target-routing names.
- Preserve prepared fact semantics and avoid consumer-shaped shortcuts.
- Keep any dependency direction compatible with existing prealloc owner
  boundaries.
- Update shared callers to use the new fact owner.

Completion check:

- Reusable current-block join-copy facts no longer require an AArch64 routing
  type or name.
- The project builds with `cmake --build --preset default`.

### Step 3: Localize AArch64 Routing Convenience

Goal: keep instruction routing convenience in target dispatch code while
consuming the shared reusable facts.

Primary targets:

- AArch64 dispatch code that currently names the prepared current-block join
  routing API
- AArch64 value materialization or publication dispatch helpers that use
  value-home/out-of-SSA predicates

Actions:

- Introduce or retain target-local routing names only where AArch64 dispatch
  needs them.
- Convert target-local routing code to depend on the shared fact predicates.
- Keep register spelling, scratch policy, hazard policy, and final emission in
  AArch64 code.
- Examine nearby join-copy routing cases, not only the first failing consumer.

Completion check:

- AArch64 routing still has the convenience it needs, but no shared prealloc API
  advertises target-local instruction routing as a reusable fact.

### Step 4: Split Value-home and Out-of-SSA Predicate Roles

Goal: separate reusable value-home/out-of-SSA fact predicates from routing uses
that only exist to choose AArch64 instructions.

Primary targets:

- Shared prealloc helpers referenced by current-block join-copy routing
- AArch64 dispatch call sites using those helpers

Actions:

- Identify predicates that describe reusable dataflow or publication facts.
- Keep reusable predicates under shared ownership and target routing adapters
  in AArch64 code.
- Avoid expanding the split into unrelated return-chain, stack-source, or
  publication facade work.

Completion check:

- Predicate names and owners make clear whether they are reusable facts or
  routing conveniences.

### Step 5: Validate and Prepare Handoff

Goal: prove the owner split and leave execution state ready for supervisor
acceptance.

Actions:

- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- If shared publication/control-flow facts changed, run full CTest:
  `ctest --test-dir build --output-on-failure`.
- Update `todo.md` with the latest completed packet, proof commands, and any
  remaining watchouts.

Completion check:

- Build and required tests pass, or blockers are recorded with exact failing
  commands.
- Reusable join-copy facts and target routing convenience no longer appear as
  one undifferentiated prepared-lookup API.
