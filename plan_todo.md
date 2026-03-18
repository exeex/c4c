# Plan Todo

## Goal

Refactor template-reduction ownership so AST preprocessing becomes a seed/todo builder,
while HIR gradually takes over the work that currently exceeds AST's intended role.

This migration should be incremental:

- keep old behavior working during the transition
- introduce new paths alongside old ones first
- only delete old paths after behavior is proven equivalent

## Phase 1: Move AST Overreach Into HIR, But Keep Old Paths

### Objective

Identify AST work that exceeds seed-collection responsibility and mirror that logic into
HIR-owned structures and helpers, without removing the current AST-driven behavior yet.

### Scope

- keep AST consteval behavior unchanged
- keep current test behavior unchanged
- add new HIR-side data flow in parallel with old AST-side flow

### Tasks

1. formalize AST seed data ✅

- `TemplateSeedWorkItem` struct with origin, bindings, nttp, mangled, spec_key
- `TemplateSeedOrigin` enum: DirectCall, EnclosingTemplateExpansion, DeducedCall, ConstevalSeed, ConstevalEnclosingExpansion
- all seed items recorded via `record_instance()` → `record_seed_work()`

2. identify AST responsibilities that should migrate ✅

- AST-side fixpoint-like consteval-template seed expansion
- AST-owned realized instance bookkeeping
- AST-driven population of final template instance metadata
- AST influence over which template functions are lowered immediately

3. introduce HIR-side registry / driver scaffolding ✅

- `InstantiationRegistry` class introduced
- centralizes:
  - dedup (`has_instance`)
  - specialization identity (`make_specialization_key` via `record_instance`)
  - explicit specialization lookup (`find_specialization`, `register_specialization`)
  - realized-instance metadata (`find_instances`, `all_instances`)
  - seed work tracking (`all_seeds`)
- all AST collection and deferred HIR paths now go through `registry_`
- old `template_fn_instances_`, `template_seed_work_`, `instance_keys_`, `template_fn_specs_` replaced by single `registry_` member

4. make HIR capable of consuming AST seed work ✅

- `record_seed_only()` records seeds without realizing instances
- `realize_seeds()` promotes seeds into instances (HIR consumption path)
- Phase 1.7a calls `realize_seeds()` in the lowering pipeline
- old dual-write path preserved; realize_seeds() is no-op until decoupled

### Exit Criteria

- AST seed/todo data is complete enough for HIR to consume ✅
- HIR has a parallel path for instance bookkeeping ✅ (registry centralizes both)
- old AST-driven behavior still remains active ✅ (dual write in registry)

## Phase 2: Gradually Switch HIR To The New Path ✅

All reads already go through the registry. Old containers fully replaced.
Parity verification confirms seed/instance consistency across all 1891 tests.

## Phase 3: Prove Behavior, Then Remove Old Paths ✅

### Completed

1. validate behavior — 1891/1891 tests pass, parity holds ✅
2. remove redundant `record_instance()` from registry and converter wrapper ✅
3. rename converter wrapper `has_instance()` → `has_seed()` for clarity ✅
4. clean up transition-era comments (dual-write, old ownership model) ✅
5. registry `record_seed_only()` renamed to `record_seed()` ✅

### Final ownership model

- AST owns discovery and seed generation (via `record_seed()`)
- HIR owns realized instance lifecycle (`realize_seeds()` is the sole path)
- `InstantiationRegistry` is the single source of truth for all template bookkeeping
- AST consteval remains as-is

## Completed Steps

1. ~~introduce the first HIR-side instantiation registry helper~~ ✅
2. ~~mirror current instance bookkeeping into that helper~~ ✅
3. ~~route all AST collection and Phase 2/3 lookups through the registry~~ ✅
4. ~~add a HIR-side path that reads seed work from the registry~~ ✅
5. ~~split seed population from instance realization in the registry~~ ✅
6. ~~add parity verification~~ ✅
7. ~~Phase 2: switch HIR to new path~~ ✅ (all reads already through registry)
8. ~~Phase 3: remove old paths and clean up~~ ✅

## Status

All three phases complete. The AST refactor plan is done.
1891/1891 tests pass.
