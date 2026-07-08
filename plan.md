# Pointer-Value Memory-Use Freshness Authority Runbook

Status: Active
Source Idea: ideas/open/600_pointer_value_memory_use_freshness_authority.md
Activated After:
- ideas/closed/597_pointer_address_semantic_model_research.md
- ideas/closed/599_pointer_base_plus_offset_selected_authority.md

## Purpose

Define selected freshness authority for pointer-value indirect memory uses
before any consumer treats address legality, range proof, or target memory
operand shape as proof that the named pointer value is current.

## Goal

Make one representative pointer-value memory-use route require selected
freshness for the exact named pointer value and load/store use.

## Core Rule

Do not accept a pointer-value memory use because
`prepared_pointer_value_memory_has_proven_authority(...)`, object extent,
offset/range proof, local layout, target offset encodability, or final target
memory operand shape exists. Those facts can support address legality, but
they do not prove pointer-value freshness for the consuming instruction.

## Read First

- ideas/open/600_pointer_value_memory_use_freshness_authority.md
- docs/pointer_address_semantic_model_research/01_pointer_address_family_inventory.md
- docs/pointer_address_semantic_model_research/02_semantic_authority_and_fact_classes.md
- docs/pointer_address_semantic_model_research/03_fail_closed_rules.md
- docs/pointer_address_semantic_model_research/05_followup_recommendations.md
- ideas/closed/587_prepared_value_freshness_authority_mvp.md
- ideas/closed/589_direct_edge_publication_move_freshness_ownership.md
- ideas/closed/597_pointer_address_semantic_model_research.md
- ideas/closed/599_pointer_base_plus_offset_selected_authority.md

## Current Targets

- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/stack_layout/*`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_printer/*`
- `src/backend/bir/lir_to_bir/memory/*`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory_store_retargeting.*`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/mir/backend_aarch64_memory_operand_records_test.cpp`
- `tests/backend/mir/backend_aarch64_memory_operand_contract_test.cpp`

## Non-Goals

- Do not implement pointer base plus offset selected authority; that is closed
  in idea 599.
- Do not implement local-array or global static semantic GEP target
  consumption.
- Do not implement broad RV64, AArch64, x86, or string assembly target
  migration.
- Do not implement global symbol memory-access authority, loaded-value
  freshness, or store-source freshness except where needed to state this
  boundary.
- Do not redesign BIR memory provenance, stack layout, target memory operand
  formation, ABI lowering, or MIR view design.
- Do not change expectations, unsupported markers, allowlists, runtime
  behavior, harness behavior, diagnostics, prepared dumps, or final assembly
  text as proof.

## Working Model

- `PreparedAddressBaseKind::PointerValue` access records and
  `prepared_pointer_value_memory_has_proven_authority(...)` prove address
  legality dimensions such as base identity, layout authority, extent, offset,
  and in-bounds range.
- Pointer-value freshness is separate: the selected authority must name the
  pointer value, memory use, program point, proof, and any required support
  facts before a consumer treats the pointer value as current.
- Missing, ambiguous, stale, wrong-pointer, wrong-load/store-use, range-only,
  local-layout-only, target-shape-only, and diagnostic-only evidence must fail
  closed.
- Loaded-value freshness, store-source freshness, semantic GEP availability,
  and pointer arithmetic remain separate use-specific contracts.

## Execution Rules

- Start with audit and contract definition before wiring acceptance behavior.
- Migrate at most one representative load/store consumer in this runbook.
- Prefer shared prepared/prealloc authority helpers and freshness lookup
  vocabulary over target-local shape checks.
- Keep address legality support facts visible but insufficient by themselves.
- If the audit proves the first route depends on loaded/store-source
  freshness, semantic GEP target consumption, or MIR view design, record that
  in `todo.md` and stop for lifecycle review.
- For code-changing steps, run the build proof selected by the supervisor and
  the narrow tests touched by the packet.

## Step 1. Audit Pointer-Value Memory Producers And Consumers

Goal: identify the prepared producers, support facts, and representative
load/store consumers around pointer-value indirect memory uses.

Actions:

- Inspect BIR/lowering producers that create `MemoryAddress::BaseKind::PointerValue`
  and pointer-value provenance.
- Inspect prepared memory access publication and address legality checks,
  especially `PreparedAddressBaseKind::PointerValue` and
  `prepared_pointer_value_memory_has_proven_authority(...)`.
- Inspect shared-prealloc publication, lookup, printer, and stack-layout
  surfaces that carry pointer-value memory facts.
- Inspect RV64 and AArch64 pointer-value memory consumers only enough to
  classify representative candidates, target-consume-only paths, rejections,
  or deferred target migrations.
- Choose one representative shared prepared/prealloc consumer route, or record
  why implementation should stop at contract/audit state.

Completion Check:

- `todo.md` names audited producers and consumers, the proposed representative
  route, and which target or value-freshness families remain out of scope.

## Step 2. Define Pointer-Value Memory Freshness Authority

Goal: make the selected freshness rule explicit before changing acceptance.

Actions:

- Decide the freshness use kind, source kind, proof kind, and rank for
  pointer-value memory-use freshness.
- State how pointer value identity, load/store use, program point, offset/range
  coordinate, provenance base, layout authority, and target shape are
  represented or rejected.
- Define which facts are required support facts but insufficient alone:
  address base kind, pointer value name/id, object extent, range proof,
  layout authority, local layout, target offset encodability, target memory
  operand shape, diagnostics, and dumps.
- Define fail-closed statuses or diagnostics for missing, ambiguous, stale,
  wrong-pointer, wrong-use, range-only, layout-only, and target-shape-only
  authority.

Completion Check:

- The ownership rule is recorded in `todo.md` and, if implementation proceeds,
  encoded in names/helpers that cannot be confused with pointer arithmetic,
  branch, edge-publication, move-bundle, select-carrier, loaded-value, or
  store-source freshness.

## Step 3. Migrate One Representative Consumer

Goal: require selected pointer-value memory-use freshness before one
representative consumer accepts the pointer value as current for a memory use.

Actions:

- Add or reuse the minimal freshness vocabulary required by Step 2.
- Publish, collect, or query the selected authority from existing semantic
  prepared facts only.
- Require the selected authority before accepting the representative
  pointer-value load/store use.
- Preserve address legality checks as support, not authority.
- Preserve rejection for missing, ambiguous, stale, wrong-pointer, wrong-use,
  range-only, layout-only, target-shape-only, and structurally complete but
  freshness-less evidence.
- Keep broad target migration, loaded-value freshness, store-source freshness,
  and semantic GEP target consumption out of this step.

Completion Check:

- The migrated consumer accepts only when selected authority matches the exact
  pointer value, memory use, program point, proof, support facts, and rank
  required by the contract.

## Step 4. Prove Fail-Closed Behavior

Goal: prove the migrated route uses explicit selected freshness and not range,
layout, or target-shape facts.

Actions:

- Add focused tests or prepared dump assertions for accepted explicit
  pointer-value memory-use freshness.
- Cover missing/no-candidate, ambiguous, stale, wrong-pointer, wrong-use,
  range-only, local-layout-only, target-shape-only, and freshness-less support
  facts where practical.
- Vary pointer value name, memory instruction use, offset/range, provenance
  base, layout authority, and target operand shape.
- Keep diagnostics precise enough to distinguish missing pointer-value
  freshness from unrelated unsupported route failures.
- Re-run existing freshness authority tests or the closest repo-native subset
  that covers 587, 589, and the touched prepared/prealloc bucket.

Completion Check:

- Fresh proof shows address legality, range/layout proof, object extent, offset
  encodability, and target memory operand shape remain insufficient without
  selected pointer-value memory-use freshness.

## Step 5. Closure Inventory And Follow-Up Decision

Goal: prepare source-idea closure without broadening this route.

Actions:

- Record audited producers and consumers.
- Record the selected pointer-value memory-use ownership rule and required
  support facts.
- Identify which representative consumer was migrated, if any.
- Identify which stale, wrong-pointer, wrong-use, range-only, layout-only, and
  target-shape-only cases fail closed.
- Record adjacent families that remain separate: loaded-value freshness,
  store-source freshness, pointer arithmetic, semantic GEP target consumption,
  global symbol memory freshness, and broad target migration.
- Run the supervisor-selected broader validation or regression guard before
  closure.

Completion Check:

- `todo.md` contains the closure inventory needed to answer every closure-note
  requirement from the source idea.

## Acceptance Check

- The audited pointer-value memory producer and consumer set is recorded.
- A selected pointer-value memory-use freshness rule names pointer value,
  load/store use, program point, proof, rank, and required support facts.
- At most one representative consumer is migrated to require selected
  freshness before semantic acceptance.
- Missing, ambiguous, stale, wrong-pointer, wrong-use, range-only,
  local-layout-only, target-shape-only, and support-only evidence fail closed.
- Focused proof demonstrates range/layout proof and target memory operand shape
  do not substitute for selected pointer-value freshness.
