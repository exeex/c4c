# AST Refactor Plan 2

## Goal

Restructure the HIR frontend files so the pipeline clearly reflects the intended
compile-time model:

1. AST is lowered into an initial HIR form
2. compile-time-required work is normalized by the compile-time engine
3. the public HIR entrypoint orchestrates the pipeline instead of owning the details

This plan focuses on file and flow separation first.
It does not yet require deeper behavior changes.

## Why A Second Refactor Plan

The first refactor plan focused on responsibility boundaries:

- AST should act like a seed/todo builder
- HIR-side logic should increasingly own compile-time reduction

This second plan narrows the immediate task to something more mechanical:

- separate the files by role
- make the pipeline visible in code structure
- move toward the final architecture without changing semantics yet

## Target File Roles

### `src/frontend/hir/ast_to_hir.*`

This file pair should return to its literal responsibility:

- AST to initial HIR construction

It should own:

- AST walking and typed lowering
- initial HIR node creation
- preservation of compile-time-related nodes/metadata
- AST-side seed collection that is still needed during transition

It should not appear to be the full owner of:

- the public HIR pipeline
- compile-time normalization policy
- the final compile-time reduction loop

### `src/frontend/hir/compile_time_engine.*`

This file pair should be the home of compile-time normalization mechanics.

It should own:

- compile-time reduction loop
- compile-time engine state
- template instantiation bookkeeping
- seed/todo consumption
- convergence / non-convergence logic
- deferred compile-time completion

This is the main place where the "compile-time computation unification" goal
should become visible.

### `src/frontend/hir/hir.*`

This file pair should become the public pipeline entrypoint for the HIR subsystem.

It should own:

- the top-level HIR build flow
- calling AST-to-initial-HIR construction
- calling compile-time normalization
- returning the final HIR module

It should not own:

- detailed AST lowering logic
- detailed compile-time engine internals

Conceptually this file should play the same role for HIR that
`src/frontend/sema/sema.cpp` plays for sema:

- pipeline coordinator
- thin orchestration layer

## Desired Pipeline Shape

The intended code-level pipeline is:

```txt
AST
  -> ast_to_hir.build_initial_hir(...)
  -> compile_time_engine.run_compile_time_engine(...)
  -> hir.build_hir(...)
```

More precisely:

1. `ast_to_hir.*` builds the initial mixed compile-time/runtime HIR
2. `compile_time_engine.*` reduces compile-time-required work
3. `hir.*` is the public entrypoint that wires those pieces together

## Immediate Refactor Objective

The first step is not to redesign the logic.
The first step is to make the file boundaries match the intended roles.

That means:

- move detailed AST/HIR lowering logic out of `hir.cpp`
- restore `ast_to_hir.cpp` as the implementation home for AST lowering
- keep `hir.cpp` as a thin orchestrator
- keep behavior stable while this split happens

## Proposed Transition Strategy

### Step 1: Restore Physical Separation

Move the detailed lowering implementation currently living in `hir.cpp`
back into `ast_to_hir.cpp`.

During this step:

- keep exported APIs stable or provide compatibility aliases
- do not intentionally change behavior
- keep current compile-time behavior intact

Expected result:

- `ast_to_hir.cpp` becomes large again temporarily
- `hir.cpp` becomes much smaller and easier to understand

That is acceptable because the main point is role clarity.

### Step 2: Make `hir.cpp` A Thin Orchestrator

After physical separation, `hir.cpp` should mostly do:

1. call AST-to-HIR construction
2. call compile-time engine
3. return the module

It may also:

- apply future materialization entrypoints
- expose the public API used by sema or tools

But it should avoid re-growing detailed logic.

### Step 3: Keep Old Internal Behavior During Split

Even after the files are separated again:

- AST-side seed collection can remain
- current callbacks can remain
- old instance bookkeeping can remain

The purpose of this plan is file and flow clarity first, not semantic cleanup.

### Step 4: Only After The Split, Start Internal Cleanup

Once the file roles are restored:

- move instance bookkeeping into `compile_time_engine.*`
- shrink AST-side ownership
- make `compile_time_engine.*` the true owner of compile-time reduction state

That deeper work belongs to later phases.

## Recommended API Shape

### In `ast_to_hir.hpp`

Introduce or rename toward an API with "initial HIR" semantics.

Example direction:

- `build_initial_hir(...)`

This name makes it clear that the result is not necessarily fully normalized yet.

### In `hir.hpp`

Keep the public subsystem entrypoint:

- `build_hir(...)`

Meaning:

- build the initial HIR
- run compile-time normalization
- return the resulting module

This distinction helps communicate the two-stage model:

- initial construction
- compile-time normalization

### In `compile_time_engine.hpp`

Keep the engine entrypoint explicit:

- `run_compile_time_engine(...)`

Long term this should operate on explicit engine state rather than relying on
ad hoc builder-owned internal state.

## Responsibility Matrix

### `ast_to_hir.*`

Owns:

- syntax-to-HIR construction
- compile-time node preservation
- transition-period AST seed extraction

Does not own:

- final compile-time normalization semantics
- compile-time engine convergence policy

### `compile_time_engine.*`

Owns:

- compile-time reduction state
- iterative reduction
- deferred completion
- normalization diagnostics

Does not own:

- raw AST walking
- general HIR construction

### `hir.*`

Owns:

- pipeline orchestration
- public subsystem entrypoint

Does not own:

- detailed lowering implementation
- detailed reduction implementation

## Success Criteria

- `hir.cpp` becomes a thin orchestration file
- `ast_to_hir.cpp` again clearly owns AST-to-initial-HIR construction
- `compile_time_engine.cpp` is clearly the place for compile-time normalization
- file names and code layout reflect the intended compile-time architecture
- behavior remains stable during this structural split

## Immediate Next Steps

1. move detailed lowering implementation from `hir.cpp` back to `ast_to_hir.cpp`
2. define a dedicated "initial HIR" builder API in `ast_to_hir.hpp`
3. keep `build_hir(...)` in `hir.cpp` as the pipeline entrypoint
4. only after that, begin moving template instance state into `compile_time_engine.*`
