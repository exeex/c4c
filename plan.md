# Lazy Template Type Instantiation Plan

## Purpose

This document is for an execution-oriented AI agent.

Read it as a runbook, not as architecture prose.

Do not redesign the system.
Do not start by editing parser files unless a step explicitly tells you to.
Do not try to solve all template bugs in one diff.


## One-Sentence Goal

Make dependent template type resolution use-site driven and engine-driven:

- use sites enqueue pending type work
- the compile-time engine owns the retry loop
- `ast_to_hir.cpp` becomes a local helper layer, not the control loop


## Current Reality

As of 2026-03-24, the codebase already has the first pieces of this plan:

- pending template type work item types already exist in
  [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- `CompileTimeState::record_pending_template_type(...)` already exists
- the engine already runs a `PendingTemplateTypeStep` in
  [compile_time_engine.cpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.cpp)
- `Lowerer::instantiate_deferred_template_type(...)` already exists in
  [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- use-site seeding already exists in some places through
  `seed_pending_template_type(...)`

What is still wrong:

- too much real resolution logic still lives inside
  `Lowerer::resolve_pending_tpl_struct(...)`
- type work is seeded only in some use sites, not consistently
- the engine owns the loop, but not enough of the decomposition
- blocked work still depends on helper recursion instead of explicit,
  smaller follow-up work


## Files You Will Touch

Primary files:

1. [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- add missing use-site seeding
- split or shrink `resolve_pending_tpl_struct(...)`
- keep helper behavior local

2. [compile_time_engine.cpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.cpp)
- extend the pending-template-type step only if engine control behavior must
  change

3. [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- touch only if you need a new work item field, helper, or status plumbing

Do not start in:

- parser files
- unrelated HIR metadata files


## Current Entry Points

Before editing, understand these exact entry points:

1. Pending type work creation:
- `CompileTimeState::record_pending_template_type(...)` in
  [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)

2. Use-site seeding helper:
- `Lowerer::seed_pending_template_type(...)` in
  [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

3. Engine loop:
- `PendingTemplateTypeStep` in
  [compile_time_engine.cpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.cpp)
- `run_compile_time_engine(...)` in
  [compile_time_engine.cpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.cpp)

4. Deferred type callback:
- `Lowerer::instantiate_deferred_template_type(...)` in
  [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

5. Current monolith that still does too much:
- `Lowerer::resolve_pending_tpl_struct(...)` in
  [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)


## What “Done” Means

This plan is complete when all of these are true:

1. every important dependent type use site enqueues pending template type work
2. the engine retry loop, not ad hoc recursion, is the primary retry mechanism
3. blocked type work spawns narrower prerequisite work where possible
4. `resolve_pending_tpl_struct(...)` no longer acts like a hidden control loop
5. unresolved diagnostics happen only after engine fixpoint exhaustion


## Rules For The Agent

Follow these rules while implementing:

1. Make one architectural move per diff.
2. Prefer moving control ownership before moving helper bodies.
3. When you see recursive “try again now” behavior, convert it into:
   - enqueue prerequisite work
   - return `blocked`
4. Do not remove eager concrete fast paths if they are obviously correct.
5. If a helper already has the right semantics but is in the wrong owner,
   keep behavior and move control later.
6. If you are unsure whether a failure is real, return `blocked`, not `terminal`.


## Resolution Contract

Every deferred template type attempt must end in one of these states:

1. `resolved`
- the pending item is concretely satisfied now

2. `blocked`
- the item is not ready yet
- the callback may enqueue more specific prerequisite work
- this is not a user-facing error yet

3. `terminal`
- the item cannot be satisfied in the current model
- this should surface only after the engine reaches fixpoint

Use this exact mental rule:

> blocked means “dependency discovered”

not

> blocked means “something went wrong”


## Current Work Item Shape

This already exists and is the base model:

```cpp
enum class PendingTemplateTypeKind {
  DeclarationType,
  OwnerStruct,
  MemberTypedef,
  BaseType,
  CastTarget,
};

struct PendingTemplateTypeWorkItem {
  PendingTemplateTypeKind kind;
  TypeSpec pending_type;
  const Node* owner_primary_def;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  SourceSpan span;
  std::string context_name;
  std::string dedup_key;
};
```

Do not replace this model wholesale unless a very small extension is necessary.


## What To Do First

Before editing code:

1. open
   [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
   at `seed_pending_template_type(...)`
2. open
   [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
   at `instantiate_deferred_template_type(...)`
3. open
   [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
   at `resolve_pending_tpl_struct(...)`
4. open
   [compile_time_engine.cpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.cpp)
   at `PendingTemplateTypeStep::run()`
5. make a list of which use sites already call `seed_pending_template_type(...)`
   and which still call eager resolution directly


## Implementation Order

Do the work in this order.
Do not skip ahead.


## Step 1. Expand Use-Site Seeding

Goal:

- when lowering needs a concrete type and sees unresolved template structure,
  it should enqueue pending work

Do this:

1. search for `resolve_pending_tpl_struct_if_needed(` in
   [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
2. for each call site, classify it as one of:
   - declaration type
   - cast target
   - owner struct
   - member typedef
   - base type
3. if the call site currently tries eager resolution without first seeding
   work, add `seed_pending_template_type(...)`
4. keep the eager attempt only as a compatibility fast path after seeding

Target areas to check first:

- local declaration lowering
- parameter / return type lowering
- struct field lowering
- cast lowering
- base realization
- member typedef follow-up paths

Completion check:

- important dependent type use sites seed work before relying on immediate
  success

Do not do in this step:

- large engine refactors
- parser edits
- deep helper splitting


## Step 2. Make Blocked Paths Spawn Explicit Owner Work

Goal:

- when a non-owner work item depends on a templated owner, it should enqueue
  owner work and return `blocked`

Primary file:

- [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Main function:

- `instantiate_deferred_template_type(...)`

Do this:

1. inspect the `MemberTypedef`, `BaseType`, `DeclarationType`, and
   `CastTarget` cases
2. if the item depends on `ts.tpl_struct_origin`, ensure the callback:
   - records `OwnerStruct` work
   - returns `blocked` if owner resolution is still pending
3. keep `terminal` only for truly invalid end states, for example:
   - member typedef lookup failed after owner is concrete
   - owner cannot be found at all

Completion check:

- blocked owner-dependent work no longer falls through into fake hard failure


## Step 3. Split `resolve_pending_tpl_struct(...)` Into Small Helpers

Goal:

- shrink the monolith so the callback can compose explicit sub-steps

Primary file:

- [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Current monolith:

- `resolve_pending_tpl_struct(...)`

Extract helpers in this order:

1. substitute deferred template arg refs
2. materialize explicit/default template args
3. evaluate deferred NTTP defaults
4. select primary/specialization pattern
5. build concrete instantiation/mangled name
6. instantiate concrete struct/class metadata

Rules:

- each helper should do one local action
- helpers should not become mini control loops
- helpers should return enough information for the caller to decide
  `resolved` / `blocked` / `terminal`

Completion check:

- `resolve_pending_tpl_struct(...)` becomes a thin coordinator or disappears


## Step 4. Move Decision-Making Out Of Recursive Helpers

Goal:

- helpers may compute
- the engine callback decides what to enqueue next

Do this:

1. whenever a helper currently “tries more things” recursively after a miss,
   stop and turn that into:
   - enqueue prerequisite work
   - return `blocked`
2. keep helper-local recursion only for clearly local expression parsing, not
   for compile-time scheduling

Common smell that means “stop and refactor”:

- one helper both discovers missing prerequisites and immediately retries the
  whole resolution chain

Completion check:

- retry behavior is visible in the engine worklist model, not hidden in helper
  recursion


## Step 5. Tighten Engine Progress Semantics Only If Needed

Goal:

- only touch the engine loop if the current blocked/spawned-new-work accounting
  is insufficient

Primary file:

- [compile_time_engine.cpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.cpp)

Current behavior already exists:

- pending items are retried every iteration
- `resolved`, `blocked`, and `terminal` are already tracked
- `spawned_new_work` already contributes to progress detection

Only edit this file if you need one of these:

- more precise pending diagnostics
- more accurate progress accounting
- stronger separation between blocked and terminal reporting

Do not rewrite the whole engine loop just because it looks old.


## What Not To Change In The First Diff

Do not do these in the first serious patch:

- replace all string transport fields with structured identity
- redesign parser preservation of deferred NTTP expressions
- remove all eager resolution code
- invent partial ordering or richer template semantics
- change unrelated template-function identity code

Those are separate tracks.


## How To Classify Outcomes While Coding

Use this table:

1. Owner struct not materialized yet
- enqueue owner work
- return `blocked`

2. Owner materialized, member typedef exists
- return `resolved`

3. Owner materialized, member typedef missing
- return `terminal`

4. Deferred NTTP default still depends on unavailable concrete bindings
- enqueue narrower prerequisite work if possible
- otherwise return `blocked`

5. Specialization/body selection can be made now
- continue locally

6. No template struct primary exists for the claimed owner
- return `terminal`


## Minimum Test Strategy

After each meaningful diff, run at least the nearby template tests.

Start with:

- `cpp_positive_sema_template_nttp_default_runtime_cpp`
- `cpp_positive_sema_template_alias_deferred_nttp_expr_runtime_cpp`
- `cpp_positive_sema_eastl_slice6_template_defaults_and_refqual_cpp`

Then run the broader targeted set if your diff changes engine control flow:

- `cpp_positive_sema_specialization_identity_cpp`
- `cpp_hir_specialization_key`
- `cpp_hir_template_call_info_dump`
- `cpp_positive_sema_template_arg_deduction_cpp`
- `cpp_positive_sema_template_default_args_cpp`
- `cpp_positive_sema_template_nttp_cpp`
- `cpp_positive_sema_template_bool_nttp_cpp`
- `cpp_positive_sema_template_char_nttp_cpp`
- `cpp_positive_sema_template_integral_nttp_types_cpp`
- `cpp_positive_sema_template_nttp_forwarding_depth_cpp`
- `cpp_positive_sema_template_recursive_nttp_cpp`
- `cpp_positive_sema_consteval_template_cpp`
- `cpp_positive_sema_consteval_nested_template_cpp`
- `cpp_positive_sema_consteval_template_sizeof_cpp`
- `cpp_hir_consteval_template_dump`
- `cpp_hir_consteval_template_call_info_dump`
- `cpp_hir_consteval_template_reduction_verified`


## Good First Diff

A good first diff for this plan does exactly this:

1. adds missing `seed_pending_template_type(...)` calls at real use sites
2. keeps eager resolution as compatibility behavior
3. makes blocked owner-dependent paths return `blocked` instead of guessing
4. does not redesign identity or parser internals


## Good Second Diff

A good second diff does exactly this:

1. splits `resolve_pending_tpl_struct(...)` into small helpers
2. moves dependency discovery into explicit callback logic
3. reduces recursive retry behavior


## Abort Conditions

Stop and reassess if you find either of these:

1. a change requires parser AST shape changes to make progress
2. a change requires structured template identity for correctness rather than
   just for cleanup

If that happens:

- make the smallest safe compatibility patch
- update this plan
- do not silently redesign the system


## Short Version

If you only remember one sequence, remember this:

1. seed pending type work at use sites
2. let the engine callback own `resolved` / `blocked` / `terminal`
3. spawn owner/member prerequisite work instead of recursing blindly
4. split `resolve_pending_tpl_struct(...)` into small local helpers
5. keep parser and identity refactors out of scope unless truly required
