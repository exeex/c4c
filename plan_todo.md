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
- **Phase 1.1 (2026-03-18):** Extracted `InstantiationRegistry` class, `TemplateSeedOrigin`,
  `TemplateSeedWorkItem`, `TemplateInstance` types, and mangling utilities
  (`mangle_template_name`, `type_suffix_for_mangling`, `bindings_are_concrete`)
  from `ast_to_hir.cpp` into `compile_time_engine.hpp` as public engine-owned types.
  `ast_to_hir.cpp` now uses the shared definitions from the engine header.
- **Phase 1.2 (2026-03-18):** Introduced `CompileTimeState` struct in `compile_time_engine.hpp`
  wrapping `InstantiationRegistry`.  Lowerer now owns a `shared_ptr<CompileTimeState>`
  (initialized inline) with a convenience `registry_` reference alias.  `CompileTimeState`
  is passed through the pipeline: `InitialHirBuildResult.ct_state` →
  `run_compile_time_engine(module, ct_state, ...)`.  Engine accepts but does not yet
  read from it — future phases will switch reads to engine-owned state.
- **Phase 1.3 (2026-03-18):** Engine now uses `ct_state->registry` for its own queries.
  `TemplateInstantiationStep` records deferred instances in the registry via `record_seed()`
  + `realize_seeds()`.  `CompileTimeEngineStats` reports registry parity (seeds vs instances).
  `CompileTimeState::dump()` added for debug visibility.  `format_compile_time_stats()`
  includes registry parity line.

- **Phase 2.1 (2026-03-18):** Added `record_deferred_instance()` and
  `instances_to_hir_metadata()` methods to `CompileTimeState`.
  `TemplateInstantiationStep` now delegates deferred-instance bookkeeping
  (seed recording + realization + HirTemplateInstantiation creation) to
  `ct_state->record_deferred_instance()` instead of manual registry calls.
  Lowerer's Phase 1.7b metadata population replaced with single call to
  `ct_state_->instances_to_hir_metadata()`.  Engine and Lowerer both route
  instance lifecycle through CompileTimeState API rather than raw registry.

- **Phase 2.2 (2026-03-18):** Made three pipeline stages explicit in code.
  Added `HirPipelineStage` enum (`Initial`, `Normalized`, `Materialized`) to `ir.hpp`.
  `Module::pipeline_stage` tracks current stage.  `build_hir()` now explicitly
  orchestrates all three stages: build_initial_hir → run_compile_time_engine →
  materialize_ready_functions, advancing `pipeline_stage` after each.
  Materialization stats (`materialized_functions`, `non_materialized_functions`)
  recorded in `Module::CompileTimeInfo`.

- **Phase 2.3 (2026-03-18):** Moved template/consteval function definition
  awareness into `CompileTimeState`.  Added `register_template_def()`,
  `register_consteval_def()`, `has_template_def()`, `has_consteval_def()`,
  `find_template_def()`, `find_consteval_def()` methods.  Lowerer registers
  defs in `ct_state_` during initial construction (Phase 1.5/1.6).
  `TemplateInstantiationStep` and `PendingConstevalEvalStep` now pre-check
  definition availability via `ct_state` before invoking callbacks.
  `CompileTimeEngineStats` reports `template_defs_known` / `consteval_defs_known`.
  Engine steps have direct visibility into what definitions exist rather than
  blindly probing opaque Lowerer callbacks.

- **Phase 2.4 (2026-03-18):** Moved consteval evaluation into engine-owned state.
  Added `register_enum_const()`, `register_const_int_binding()`, and
  `evaluate_consteval()` methods to `CompileTimeState`.  Lowerer mirrors
  enum constants and global const-int bindings into `ct_state_` during
  initial HIR construction.  `PendingConstevalEvalStep` now prefers
  `ct_state->evaluate_consteval()` directly when ct_state is available,
  falling back to the `DeferredConstevalEvalFn` callback only when no
  ct_state is provided.  The compile-time engine can now evaluate consteval
  expressions without routing through opaque Lowerer callbacks.

- **Phase 2.5 (2026-03-18):** Moved pre-checks and post-metadata out of the
  `DeferredInstantiateFn` callback into the engine's `TemplateInstantiationStep`.
  Added `is_consteval_template()` to `CompileTimeState` — engine now skips
  consteval templates before invoking the callback (previously checked inside
  Lowerer).  Engine now sets `template_origin` and `spec_key` on newly-lowered
  functions after the callback returns (previously set inside Lowerer).
  `Lowerer::instantiate_deferred_template()` is now a pure lowering operation:
  def lookup → specialization check → `lower_function()` call.

- **Phase 3.1 (2026-03-18):** Removed legacy compatibility aliases and fallback paths.
  Deleted `lower_ast_to_hir()` (hir.hpp) and `run_compile_time_reduction()`
  (compile_time_engine.hpp) — neither had call sites.  Removed
  `DeferredConstevalEvalFn` callback type and parameter: consteval evaluation
  now uses `CompileTimeState::evaluate_consteval()` exclusively; the Lowerer's
  `evaluate_deferred_consteval()` method (duplicate of engine-owned logic) deleted.
  Removed null `ct_state` fallback metadata construction in
  `TemplateInstantiationStep`.  Updated `README.md` with current three-stage
  pipeline architecture and accurate file roles.

Not completed:

- DeferredInstantiateFn still captures the Lowerer for `lower_function()` — this is
  fundamentally required since only the Lowerer can lower AST to HIR
- Lowerer still maintains duplicate `consteval_fns_` / `template_fn_defs_` maps
  (used during initial HIR construction; engine has copies in CompileTimeState)
- `CompileTimePassStats` type alias could be removed
- Phase 3 tasks 2-3: further legacy removal if beneficial

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

1. ~~introduce engine-owned type definitions~~ (done: types now in compile_time_engine.hpp)
2. ~~move `InstantiationRegistry` into `CompileTimeState`, pass through pipeline~~ (done: Phase 1.2)
3. ~~add debug visibility for seed work vs realized instances via engine-owned state~~ (done: Phase 1.3)
4. ~~have `run_compile_time_engine` use `ct_state->registry` for its own queries~~ (done: Phase 1.3)
5. ~~switch reads to engine-owned path via CompileTimeState API~~ (done: Phase 2.1)
6. ~~make three pipeline stages (initial / normalized / materialized) explicit in code~~ (done: Phase 2.2)
7. ~~reduce reliance on AST-owned callback semantics~~ (done: Phase 2.3 — definition registries in CompileTimeState)
8. ~~move consteval evaluator into engine-owned state~~ (done: Phase 2.4 — CompileTimeState::evaluate_consteval)
9. ~~further reduce callback surface for template instantiation~~ (done: Phase 2.5 — engine owns pre-checks + post-metadata)
10. ~~Phase 3 slice 1: remove compatibility aliases, DeferredConstevalEvalFn, null ct_state fallbacks, update README~~ (done: Phase 3.1)
11. Phase 3 remaining: evaluate further legacy removal (duplicate Lowerer maps, CompileTimePassStats alias)
