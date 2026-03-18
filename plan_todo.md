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

4. make HIR capable of consuming AST seed work

- add a path that reads seed work from the registry
- translate seeds into HIR-side todo/work items
- do not yet disable the old AST-driven realized-instance path

### Exit Criteria

- AST seed/todo data is complete enough for HIR to consume ✅
- HIR has a parallel path for instance bookkeeping ✅ (registry centralizes both)
- old AST-driven behavior still remains active ✅ (dual write in registry)

## Phase 2: Gradually Switch HIR To The New Path

### Objective

Move step by step from AST-owned realized instantiation behavior to HIR-owned processing,
while keeping both systems available long enough to compare results safely.

### Tasks

1. make HIR prefer the new source of truth in limited areas

- start with metadata updates
- then instance bookkeeping
- then deferred discovery / completion

2. compare old and new paths during transition

- add debug assertions or dumps where useful
- compare:
  - discovered seeds
  - realized instances
  - specialization keys
  - explicit specialization resolution

3. narrow AST responsibility

- keep AST as:
  - definition index builder
  - specialization index builder
  - seed collector
  - deduction helper
- stop treating AST containers as the authoritative realized-instance store

4. route lowering decisions through HIR-side ownership

- reduce direct dependence on `template_fn_instances_` as the final authority
- have lowering consult the new HIR-side registry/driver first
- keep fallback to old behavior until parity is proven

### Exit Criteria

- HIR path can reproduce current realized instances
- lowering can use the new path with fallback retained
- AST is no longer conceptually the owner of realized instantiation results

## Phase 3: Prove Behavior, Then Remove Old Paths

### Objective

After parity is established, remove the legacy AST-owned realized-instantiation path and
keep only the cleaner responsibility split.

### Tasks

1. validate behavior thoroughly

- build successfully
- run relevant template / consteval tests
- compare `--dump-hir` output where useful
- ensure no regression in emitted `.ll`

2. remove old AST-owned realized-instance path

- delete legacy bookkeeping that duplicates HIR-owned state
- remove temporary fallback logic
- remove comments that describe the old mixed-ownership model

3. keep only the final responsibility split

- AST owns discovery and seed generation
- HIR owns realized instance lifecycle and deferred completion
- AST consteval remains as-is unless later work chooses to change it

### Exit Criteria

- tests pass
- old path is removed
- code clearly reflects the new ownership model

## Immediate Next Steps

1. ~~introduce the first HIR-side instantiation registry helper~~ ✅ done
2. ~~mirror current instance bookkeeping into that helper~~ ✅ done (dual write)
3. ~~route all AST collection and Phase 2/3 lookups through the registry~~ ✅ done
4. add a HIR-side path that reads seed work from the registry (Phase 1 task 4)
5. split seed population from instance realization in the registry
   - seeds populated during AST discovery
   - instances populated only during HIR reduction
6. add debug dump or comparison output to verify seed/instance parity
