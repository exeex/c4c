# AArch64 Dispatch Value Materialization Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md

## Purpose

Repair duplicate value-materialization authority in AArch64 dispatch value
materialization by routing semantic source, producer, publication, memory,
global, select-chain, and local-slot address facts through prepared records or
narrow shared queries.

## Goal

Make `dispatch_value_materialization.cpp` consume shared prepared authority for
non-edge scalar materialization instead of growing same-block producer
recursion, value-name recovery, global-symbol reconstruction, or select-chain
fallbacks as local source-of-truth paths.

## Core Rule

Do not repair dispatch value materialization through testcase-shaped producer
shortcuts, deeper same-block recursion, global-name matching, expectation
downgrades, or helper renames. Accepted changes must move semantic authority
into existing prepared facts or a narrow shared prepared query.

## Read First

- `ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- Existing prepared source/home/publication surfaces before adding new ones:
  - `src/backend/prealloc/prepared_lookups.cpp`
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/publication_plans.cpp`
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/regalloc/consumer_moves.cpp`
  - `src/backend/prealloc/regalloc/consumer_moves.hpp`

## Current Targets

- Primary owned file:
  - `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- Prepared lookup, publication-plan, memory-access, or value-home files may be
  touched only when a missing shared query is required for this dispatch
  materialization contract.
- Select-chain producer lookup files may be touched only for Step 4 when the
  recursive shared select-chain emitter still owns duplicate producer
  selection:
  - `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`

## Non-Goals

- Do not change recursive operand emission, scratch/read-write hazard checks,
  AArch64 instruction spelling, load/global address spelling, compare/select
  instruction spelling, GOT/direct mechanics, or local-slot frame-offset math
  except to consume a shared semantic contract.
- Do not fold calls, ALU, comparison, memory, publication, or FP/global
  consumer repairs into this runbook.
- Do not treat deletion of fallback lines as success unless duplicate semantic
  recovery is actually removed or replaced by prepared authority.
- Do not edit `globals.cpp` or `fp_value_materialization.cpp`; those belong to
  the separate follow-up source idea `54`.

## Working Model

- `emit_value_publication_to_register` must materialize values from prepared
  source/home facts or narrowly shared source-producer queries, not from local
  same-block semantic reconstruction.
- Current-block entry publication and prepared value-home helpers are valid as
  consumers of prepared block-entry/value-home facts.
- Local-load materialization should stay anchored in `PreparedMemoryAccess`,
  `PreparedAddressingFunction`, recovered narrow store-source facts, and
  prepared store-source publication plans.
- Global-load materialization needs a shared prepared global address policy if
  existing prepared address materialization cannot carry GOT/direct policy.
- Select-chain materialization for non-edge/non-store consumers should consume
  the same shared authority chosen for other scalar materialization paths.
- Local-slot address materialization should consume the same local-slot
  address/frame-offset authority selected by the publication repair.

## Execution Rules

- Inspect existing prepared records and lookup helpers before adding a query.
- Add one narrow shared query only after proving existing prepared value-home,
  block-entry publication, edge source-producer, scalar-publication,
  memory-access, address-materialization, or store-source facts cannot answer
  the dispatch consumer.
- Keep changes behavior-preserving except for selecting the correct authority
  source.
- Keep packet proof focused first: build, then a supervisor-selected dispatch
  value-materialization or backend subset. Escalate to broader backend proof if
  shared prepared lookup, publication-plan, memory, or consumer-move behavior
  changes.
- Record routine packet progress and proof in `todo.md`, not by rewriting this
  runbook.

## Step 1 - Audit Dispatch Materialization Fallbacks

Goal: classify each duplicate-authority fallback in
`dispatch_value_materialization.cpp` and map it to an existing prepared fact or
a justified missing shared query.

Primary target:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

Actions:

- Inspect `emit_value_publication_to_register`, especially the same-block
  producer path through `find_same_block_named_producer` and recursive
  cast/binary/load/select lowering.
- Inspect current-block entry publication helpers and confirm they remain
  consumers of `PreparedBlockEntryPublication` and `PreparedValueHome`.
- Inspect load-local, load-global, select-chain, and local-slot address paths
  and record which prepared facts are already available at each consumer.
- Identify the first bounded fallback whose authority can be repaired without
  expanding scope outside dispatch value materialization.

Completion check:

- `todo.md` records the selected first fallback, the prepared authority it
  should consume or the exact missing query, and the focused proof subset the
  supervisor should delegate for implementation.

## Step 2 - Replace Same-Block Scalar Producer Recovery

Goal: make non-edge scalar publication materialization consume prepared
producer/home facts instead of local same-block producer recursion.

Primary target:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

Actions:

- Prefer `PreparedEdgePublicationSourceProducerLookups`,
  `PreparedEdgePublicationSourceProducer`, `PreparedScalarPublicationPlan`, and
  `PreparedValueHome` where they already answer the source producer question.
- Add or consume a prepared scalar materialization/source-producer query by
  value and instruction index only if non-edge callers cannot consume existing
  prepared producers.
- Remove or bypass duplicate same-block semantic reconstruction only after the
  prepared contract is available.

Completion check:

- Same-block named producer recursion no longer grows as the source of truth
  for scalar materialization at the dispatch root. Recursive select-chain
  producer fallback through `emit_select_chain_value_to_register` is not a
  Step 2 closure blocker; it belongs to Step 4 shared select-chain producer
  authority. Focused build plus test proof is recorded in `todo.md`.

## Step 3 - Repair Load-Local Materialization Authority

Goal: keep load-local source materialization anchored in prepared memory and
recovered store-source facts.

Primary target:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

Actions:

- Route `prepared_local_load_offset`,
  `emit_prepared_pointer_value_load_to_register`, and recovered wide-load
  source handling through `PreparedMemoryAccess`, `PreparedAddressingFunction`,
  `PreparedStoreSourcePublicationPlan`, and
  `find_prepared_recovered_narrow_store_source_for_wide_local_load`.
- Avoid neighbor scans or value-name recovery for load-local source identity.
- Preserve existing load instruction spelling, source reload sequencing, and
  scratch behavior.

Completion check:

- Load-local value materialization consumes prepared memory and recovered-source
  facts without local semantic recovery, with focused proof recorded in
  `todo.md`.

## Step 4 - Repair Global And Select-Chain Materialization Authority

Goal: make dispatch global-load and select-chain materialization consume shared
authority instead of local global-symbol or select-chain recovery.

Primary target:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`

Actions:

- For load-global materialization, consume existing prepared address
  materialization if it carries the needed symbol and GOT/direct policy.
- Add or consume a prepared global-load address/materialization query keyed by
  value or instruction only if the existing prepared address surface is
  insufficient.
- For non-edge/non-store select-chain materialization, add or consume a shared
  scalar select-chain materialization query when shared producer facts cannot
  express the route.
- If recursive select-chain materialization delegates to
  `emit_select_chain_value_to_register`, repair producer lookup at the shared
  select-chain producer/emitter boundary instead of adding another
  dispatch-local walker.
- Do not hard-code global names, direct/GOT policy, or select-chain cases.

Completion check:

- Global-load and select-chain dispatch paths consume shared authority or a
  narrow new shared query, and focused plus appropriate broader proof is
  recorded in `todo.md`.

## Step 5 - Repair Local-Slot Address Materialization Authority

Goal: make local-slot address materialization consume the shared local-slot
address/frame-offset authority selected by publication repair.

Primary target:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

Actions:

- Inspect add/sub address paths through
  `emit_local_slot_address_publication_to_register`.
- Reuse prepared local-slot address/frame-offset facts or pointer-base-plus-
  offset homes where they already cover the address publication contract.
- Add only a narrow shared query if the publication repair left a real
  dispatch consumer gap.

Completion check:

- Local-slot address materialization no longer derives frame-slot authority
  locally when prepared authority exists, and proof is recorded in `todo.md`.

## Step 6 - Validate Dispatch And Shared Prepared Behavior

Goal: prove the dispatch authority repair covers known and nearby paths without
regressing broader backend lowering.

Actions:

- Run the supervisor-selected build command before focused tests.
- Run a focused backend subset covering dispatch value publication,
  load-local, load-global, select-chain, and local-slot address materialization
  routes touched by the slice.
- Add same-feature probes when a query is generalized beyond one known case.
- Escalate to broader backend or full CTest proof if shared prepared lookup,
  publication-plan, memory-access, or consumer-move behavior changes.

Completion check:

- Focused proof is green, broader proof is green when required by blast radius,
  and `todo.md` records exact commands and log locations.
