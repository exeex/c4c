# Lazy Template Type Instantiation

## Status

Completed as a project track on 2026-03-24.


## Outcome

This line of work is considered closed.

The implemented direction is:

- dependent template type work is use-site driven
- pending template type work is recorded into compile-time state
- the compile-time engine owns the retry loop for deferred template type work
- deferred type resolution uses `resolved` / `blocked` / `terminal`
  outcomes
- helper recursion was reduced so engine-visible work, not hidden retry logic,
  is the primary control model


## What Landed

The codebase now has these pieces in place:

1. Pending type work items and dedup live in
   [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp).
2. Engine-driven retry for pending template types lives in
   [compile_time_engine.cpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.cpp).
3. Use-site seeding and deferred type callbacks live in
   [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp).
4. `resolve_pending_tpl_struct(...)` was split down enough that it no longer
   serves as the hidden global control loop.
5. EASTL-adjacent fallout discovered during this track was fixed, and the full
   suite reached `2123/2123`.


## Final Acceptance State

This route is considered complete for project planning purposes because:

1. important dependent type use sites enqueue pending template type work
2. the engine retry loop is the primary retry mechanism
3. blocked work can spawn narrower prerequisite work
4. unresolved diagnostics are deferred until fixpoint exhaustion
5. no further dedicated tracking document is needed for this migration


## Remaining Reality

Completed does not mean the template system is “finished forever”.

Still true:

- future template bugs may still appear
- structured template identity remains its own separate topic
- parser transport fields such as `tpl_struct_origin` still exist as
  compatibility state

Those are follow-on maintenance concerns, not reasons to keep this plan open.


## Where To Look If Future Work Touches This Area

Start here:

- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- [compile_time_engine.cpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.cpp)
- [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Related design note:

- [structured_template_identity_plan.md](/workspaces/c4c/ideas/open/structured_template_identity_plan.md)


## Note To Future Agents

Do not reopen this file as an active migration checklist unless a new project
explicitly decides to extend or replace the current lazy-instantiation model.
