# AArch64 Dispatch Value Materialization Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md

## Purpose

Turn `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` into
a consumer of shared prepared source, producer, publication, memory, global,
select-chain, and local-slot address facts instead of locally rediscovering
the same value-materialization authority.

## Goal

Remove or fail-close duplicate semantic source reconstruction in AArch64 value
materialization while preserving emitted behavior.

## Core Rule

Prefer prepared/shared authority over same-block producer recursion, neighbor
scans, global-name policy recovery, local-slot spelling, or expectation
downgrades as progress.

## Read First

- `ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- Prepared authority definitions and lookup helpers for:
  - `PreparedEdgePublicationSourceProducerLookups`
  - `PreparedEdgePublicationSourceProducer`
  - `PreparedScalarPublicationPlan`
  - `PreparedValueHome`
  - `PreparedBlockEntryPublication`
  - `collect_prepared_block_entry_publications`
  - `prepared_block_entry_publication_available`
  - `find_indexed_prepared_value_home`
  - `PreparedMemoryAccess`
  - `PreparedAddressingFunction`
  - `PreparedStoreSourcePublicationPlan`
  - `find_prepared_recovered_narrow_store_source_for_wide_local_load`
  - `PreparedAddressMaterialization`

## Current Targets

- `emit_value_publication_to_register`
- `find_same_block_named_producer`
- recursive cast/binary/load/select materialization from named producers
- `current_block_entry_publication_register`
- `value_has_current_block_entry_publication`
- `prepared_value_home_for_value`
- `prepared_local_load_offset`
- `emit_prepared_pointer_value_load_to_register`
- `find_prepared_recovered_narrow_store_source_for_wide_local_load`
- `find_load_global_target`
- `load_global_symbol_label`
- GOT/direct global-load emission paths
- `emit_select_chain_value_to_register`
- `emit_local_slot_address_publication_to_register`

## Non-Goals

- Do not change recursive operand emission, scratch/read-write hazard checks,
  load/global address spelling, compare/select instruction spelling, or
  AArch64-specific GOT/direct emission mechanics unless required to consume a
  shared semantic contract.
- Do not expand this idea into calls, ALU, comparison, publication, or memory
  repairs outside this file.
- Do not classify scratch-order checks alone as duplicate authority unless the
  code also reselects semantic source or home facts locally.
- Do not claim line-count reduction, helper renames, or expectation rewrites as
  capability progress.
- Do not downgrade supported-path tests to `unsupported`.

## Working Model

- Prepared source-producer and scalar-publication facts should own scalar
  materialization source identity for non-edge callers.
- Prepared block-entry publication and value-home records should own current
  block-entry and value-home availability.
- Prepared memory/access and recovered store-source facts should own load-local
  materialization.
- Prepared address/materialization authority should own global-load GOT/direct
  policy if existing records can express it; otherwise a narrow shared query
  should be added before removing local fallback behavior.
- Shared scalar select-chain authority should own non-edge/non-store
  select-chain materialization when producer facts are insufficient.
- The local-slot address authority selected by the publication repair should
  be reused here instead of adding another frame-offset recovery path.

## Execution Rules

- Keep each code-changing step behavior-preserving unless the source idea
  explicitly requires fail-closed behavior for unsupported local rediscovery.
- Before adding a new query, prove the existing prepared lookup surface cannot
  answer the required value-materialization context.
- Add only narrowly scoped shared queries for real scalar-materialization,
  global-load, select-chain, or local-slot authority gaps.
- Each implementation packet needs fresh build or compile proof. The supervisor
  chooses the exact proving subset.
- Narrow proof must cover the repaired materialization route and any retained
  fallback path for that route.

## Steps

### Step 1: Map value-materialization authority gaps

Goal: establish where `dispatch_value_materialization.cpp` already consumes
prepared authority and where it reconstructs semantic materialization facts
locally.

Primary target: `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

Actions:

- Inspect non-edge callers of `emit_value_publication_to_register` and record
  where same-block producer recursion is still the source of truth.
- Inspect block-entry/value-home helpers and confirm which paths are already
  pure consumers of prepared block-entry and value-home facts.
- Inspect load-local, load-global, select-chain, and local-slot address paths
  for local source, policy, or frame-offset recovery.
- Identify the first fallback route to repair and the smallest proof subset
  that covers it.

Completion check:

- `todo.md` names the exact first fallback route to repair and the proof subset
  the supervisor selected for that route.

### Step 2: Route non-edge scalar producer materialization through prepared source authority

Goal: stop same-block named producer recursion from growing as scalar
materialization authority for non-edge callers.

Primary target: `emit_value_publication_to_register`,
`find_same_block_named_producer`, and recursive cast/binary/load/select
lowering

Actions:

- Consume `PreparedEdgePublicationSourceProducerLookups`,
  `PreparedEdgePublicationSourceProducer`, `PreparedScalarPublicationPlan`, and
  `PreparedValueHome` where they cover non-edge scalar sources.
- Add a prepared scalar-materialization/source-producer query by value and
  instruction index only if non-edge callers cannot consume existing prepared
  producers.
- Remove or fail-close local same-block producer recursion as semantic source
  authority after the shared path exists.

Completion check:

- Non-edge scalar materialization consumes prepared producer/home authority or
  a documented shared scalar query, with proof for the repaired route.

### Step 3: Preserve block-entry and value-home helpers as prepared consumers

Goal: ensure block-entry and value-home checks remain consumers of prepared
facts, not local rediscovery.

Primary target: `current_block_entry_publication_register`,
`value_has_current_block_entry_publication`, and
`prepared_value_home_for_value`

Actions:

- Consume `PreparedBlockEntryPublication`, `PreparedValueHome`,
  `collect_prepared_block_entry_publications`,
  `prepared_block_entry_publication_available`, and
  `find_indexed_prepared_value_home`.
- Reject raw move-bundle, value-home spelling, or value-name scans as a
  replacement for prepared block-entry/value-home checks.
- Document any retained fallback as fail-closed or as an explicit shared-query
  gap in `todo.md`.

Completion check:

- Block-entry/value-home materialization decisions are prepared-fact consumers,
  with proof for any route changed in this step.

### Step 4: Route load-local materialization through prepared memory and recovered-source facts

Goal: keep load-local source materialization anchored in prepared memory and
recovered store-source authority without neighbor scans.

Primary target: the load-local branch using `prepared_local_load_offset`,
`emit_prepared_pointer_value_load_to_register`, and
`find_prepared_recovered_narrow_store_source_for_wide_local_load`

Actions:

- Consume `PreparedMemoryAccess`, `PreparedAddressingFunction`,
  `PreparedStoreSourcePublicationPlan`, and recovered narrow-store source
  facts for load-local materialization.
- Remove or fail-close neighbor scans or local reconstruction that duplicate
  prepared memory/source authority.
- Preserve final load behavior unless prepared authority proves a local
  fallback unsupported.

Completion check:

- Load-local source materialization consumes prepared memory/recovered-source
  facts, with proof for the affected route.

### Step 5: Route global-load materialization through shared address authority

Goal: prevent local global-name spelling, GOT policy, or direct-global recovery
from becoming a second semantic authority.

Primary target: `find_load_global_target`, `load_global_symbol_label`, and
GOT/direct global-load emission paths

Actions:

- Check whether `PreparedAddressMaterialization` can carry the needed
  global-load address and GOT/direct policy.
- Add or consume a prepared global-load address/materialization query keyed by
  value or instruction only if existing prepared address authority cannot cover
  the path.
- Replace local fallback behavior only after the shared authority is available
  and covered by tests.

Completion check:

- Global-load materialization consumes shared address/materialization authority,
  with proof for GOT/direct policy preservation.

### Step 6: Route non-edge select-chain materialization through shared scalar authority

Goal: stop local select-chain materialization from duplicating shared
select-chain authority for non-edge/non-store consumers.

Primary target: the select branch calling `emit_select_chain_value_to_register`
with `prepared_named_value_id`

Actions:

- Consume existing prepared producer/scalar-publication facts where they cover
  non-edge select-chain materialization.
- Add or consume a prepared scalar select-chain materialization query if shared
  producer facts are insufficient.
- Avoid direct-global or named select-chain shortcuts whose evidence is only
  one testcase.

Completion check:

- Non-edge select-chain materialization is routed through shared scalar or
  select-chain authority, with proof for the repaired path and a nearby
  same-feature route when available.

### Step 7: Reuse prepared local-slot address authority

Goal: remove local-slot address/frame-offset recovery that duplicates the
authority selected by the publication repair.

Primary target: `emit_local_slot_address_publication_to_register`

Actions:

- Reuse the prepared local-slot address/frame-offset query or authority
  selected by the publication repair.
- Prefer pointer-base-plus-offset homes or prepared address materialization
  over stack-object spelling or lane-based recovery.
- Do not add a second AArch64-only local-slot address authority path.

Completion check:

- Local-slot address materialization consumes the shared publication-repair
  authority, with proof for the affected address route.

### Step 8: Consolidate proof and close readiness

Goal: ensure the completed repair is not a testcase-overfit slice.

Primary target: lifecycle proof in `todo.md`, `test_before.log`, and
`test_after.log`

Actions:

- Confirm all remaining local fallback paths are removed, fail-closed, or
  documented as shared-query gaps with corresponding shared queries.
- Confirm no expectations were downgraded and no unsupported-path rewrites were
  used as capability proof.
- Run the supervisor-selected broader validation after the repaired
  materialization routes have landed or the repair is ready to close.

Completion check:

- `todo.md` records fresh proof covering non-edge scalar materialization,
  block-entry/value-home consumption, load-local materialization, global-load
  authority, select-chain authority, and local-slot address authority as
  applicable.
