# AArch64 Store-Source Semantic Residue Prerequisite Runbook

Status: Active
Source Idea: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md

## Purpose

Remove the remaining AArch64-local source-choice authority from
`memory_store_sources.*` so the later memory fold-back can be mechanical.

## Goal

Make AArch64 memory/store-source helpers consume shared prepared source facts
or fail closed when facts are missing, without moving the existing semantic
rediscovery into another AArch64 owner.

## Core Rule

Do not fold `memory_store_sources.*` into `memory.cpp` in this plan. This plan
exists to eliminate or target-neutralize semantic residue first.

## Read First

- `ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md`
- `ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/memory_store_sources.cpp`
- `src/backend/mir/aarch64/codegen/memory_store_sources.hpp`
- shared prepared store-source planning code
- focused store-source planning and AArch64 memory/dispatch tests

## Current Scope

- Resolve semantic source-choice residue in:
  - `find_latest_narrow_store_for_wide_local_load`
  - `store_local_recovered_narrow_store_source`
  - `store_local_value_is_byval_frame_slot_load`
  - `store_local_value_is_wide_load_from_narrow_local_store`
  - `store_local_value_has_select_producer`
  - `store_local_value_has_scalar_fp_binary_producer`
  - `select_chain_contains_direct_global_load`
  - the same-block producer fallback inside
    `emit_pointer_base_plus_offset_to_register`
- Preserve supported behavior by moving semantic authority into prepared
  planning or by retaining fail-closed behavior when prepared facts are absent.
- Keep AArch64 code responsible only for target instruction spelling,
  addressing, scratch use, and emission around prepared facts.

## Non-Goals

- Do not remove `memory_store_sources.cpp` or `memory_store_sources.hpp`.
- Do not perform idea 39's mechanical memory fold-back in this plan.
- Do not change frame layout, stack allocation, broad memory lowering, or final
  assembly printing.
- Do not fold unrelated helper families.
- Do not weaken diagnostics, downgrade tests, or remove negative coverage.

## Working Model

- Treat source choice as target-neutral semantic planning.
- Treat AArch64 memory/store-source code as a prepared-fact consumer.
- If an AArch64 helper needs a source fact that prepare does not provide, add
  the missing shared authority or fail closed explicitly.
- Keep implementation packets small because this touches prepared planning and
  AArch64 memory dispatch behavior.

## Execution Rules

- Start with a detailed ownership inventory before implementation.
- Separate local store, global store/select-chain, and pointer-base fallback
  work when they have different prepared facts.
- Preserve behavior and diagnostics for all supported cases.
- Use focused store-source planning tests and AArch64 memory/dispatch tests for
  each code-changing packet.
- Escalate to a backend bucket before lifecycle closure.

## Ordered Steps

### Step 1: Inventory Semantic Residue And Prepared Fact Gaps

Goal: identify exactly which local semantic decisions remain in AArch64 and
which prepared facts would replace them.

Primary target: `memory_store_sources.*`, shared prepared store-source
planning, and focused tests.

Actions:
- Map each blocked helper to the source fact it currently recovers locally.
- Identify existing prepared facts that already cover the case.
- Identify missing prepared facts, missing fail-closed checks, and external
  callers that will be affected.
- Record test handles for each source-choice family.

Completion check:
- `todo.md` records the helper-to-fact map, external callers, missing prepared
  facts, fail-closed expectations, test handles, and the recommended first
  semantic implementation packet.

### Step 2: Move Local Store Source Recovery To Prepared Authority

Goal: remove AArch64-local source recovery for local store publication cases.

Primary target: shared prepared store-source planning and the local-store
publication helpers in `memory_store_sources.cpp`.

Actions:
- Replace local narrow-store, byval frame-slot load, select producer,
  scalar-FP producer, and cast/source checks with prepared source authority
  where supported.
- Preserve missing-fact fail-closed behavior.
- Keep AArch64 emission code as a consumer of prepared facts.

Completion check:
- Local store publication no longer depends on AArch64 same-block semantic
  source choice, and focused store-source/AArch64 tests pass.

### Step 3: Move Global Store And Pointer-Base Recovery To Prepared Authority

Goal: remove remaining select-chain and pointer-base source rediscovery from
AArch64 store-source helpers.

Primary target: global store publication paths and pointer-base-plus-offset
materialization helpers.

Actions:
- Replace `select_chain_contains_direct_global_load`-style source recovery with
  prepared authority or fail-closed checks.
- Replace pointer-base load-local fallback in
  `emit_pointer_base_plus_offset_to_register` with prepared value-home/source
  facts or fail closed.
- Keep target instruction emission behavior unchanged for supported prepared
  cases.

Completion check:
- Global store and pointer-base paths no longer choose semantic sources
  locally, and focused store-source/AArch64 tests pass.

### Step 4: Validate Prerequisite Completion

Goal: prove idea 39 can resume as a mechanical fold-back.

Primary target: focused store-source planning and AArch64 memory/dispatch
coverage plus a backend bucket.

Actions:
- Run a fresh build after code-changing packets.
- Run focused store-source planning and AArch64 memory/dispatch tests.
- Run a backend bucket before closure.
- Search `memory_store_sources.*` for remaining same-block source-choice
  recovery before declaring the prerequisite complete.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  remaining `memory_store_sources.*` surface is pure emission around prepared
  facts or explicit fail-closed behavior.
