# Plan Todo

## Goal

Continue the HIR frontend refactor toward a clearer compile-time architecture:

- `ast_to_hir.*` builds the initial HIR
- `compile_time_engine.*` owns compile-time normalization
- `hir.*` is the public pipeline orchestrator

The structural split is already done.
The remaining work is mainly about moving ownership and semantics, while keeping
old behavior alive long enough to verify parity.

## Current Status

Completed:

- `src/frontend/hir/ast_to_hir.*` now holds AST -> initial HIR construction again
- `src/frontend/hir/hir.*` is now a thin orchestration entrypoint
- `src/frontend/hir/compile_time_engine.*` is the canonical engine naming
- seed/todo container exists: `template_seed_work_`
- seed origins are recorded
- compatibility names are still present where helpful

Not completed:

- compile-time state ownership is still mostly builder-owned
- template instance lifecycle is still centered in `ast_to_hir.cpp`
- engine is still callback-driven and thinner than the intended final design

## Phase 1: Move Compile-Time State Toward The Engine

### Objective

Keep the new file split stable, and move ownership of compile-time state away
from AST-lowering internals.

### Tasks

1. define the first explicit engine-owned state structure

- add a compile-time engine state object in `compile_time_engine.*`
- begin moving or mirroring:
  - realized template instances
  - instance dedup keys
  - specialization lookup ownership
  - seed consumption state

2. reduce builder-owned compile-time state

- stop treating `ast_to_hir.cpp` as the conceptual owner of:
  - `template_fn_instances_`
  - instance dedup rules
  - realized instance lifecycle

3. keep migration parallel

- do not delete current AST-side bookkeeping yet
- mirror data into the new engine-owned state first
- compare parity before switching reads

4. improve observability

- add dump/debug support for:
  - seed work
  - realized instances
  - parity comparison between old and new paths

### Exit Criteria

- compile-time engine has an explicit state object
- builder-owned instance state is no longer the only source of truth
- old and new state can be compared safely

## Phase 2: Make The Engine The Real Owner

### Objective

Gradually change the runtime of the pipeline so compile-time normalization logic
uses engine-owned state as the primary source of truth.

### Tasks

1. route instance realization through engine APIs

- move `record_instance` / `has_instance` style logic behind engine-owned helpers
- make `ast_to_hir.cpp` produce seeds and initial HIR, not act like the final owner

2. reduce reliance on AST-owned callback semantics

- keep callbacks during transition
- but reshape them so the engine works on explicit compile-time state rather than
  opaque builder-owned internals

3. make the three stages explicit in code and comments

- initial HIR
- normalized HIR
- materialized HIR

4. switch readers incrementally

- first metadata population
- then instance lookup
- then lowering decisions

### Exit Criteria

- engine-owned state reproduces the current realized instances
- primary reads use the new engine-owned path
- AST lowering is no longer conceptually the owner of normalization results

## Phase 3: Verify, Simplify, Remove Old Paths

### Objective

After parity is proven, remove legacy ownership paths and keep the cleaner model.

### Tasks

1. verify behavior

- build successfully
- run relevant template / consteval tests
- compare `--dump-hir` output where useful
- compare emitted `.ll` where useful

2. remove legacy ownership paths

- delete old builder-owned realized-instance bookkeeping
- delete fallback or duplicate state once parity is proven

3. remove compatibility naming when safe

- old function aliases
- old compatibility headers
- outdated comments/docs

4. update `src/frontend/hir/README.md`

### Exit Criteria

- tests pass
- old ownership path is removed
- code layout and code behavior both reflect the new model

## Immediate Next Steps

1. introduce a first engine-owned compile-time state object
2. mirror `template_fn_instances_`-style state into the engine layer
3. add debug visibility for seed work vs realized instances
4. only after parity checks, start switching reads over to the engine-owned path
