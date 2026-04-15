# Route Review: prepare vs prealloc

- Base commit: `3ebda3ca` (`[plan] reset prepare liveness ownership`)
- Why this base: it is the last canonical `[plan]` checkpoint for the current active runbook, and its diff/reset text matches the still-active `plan.md` lifecycle rather than an older activation commit.
- Commit count since base: `21`

## Findings

### High

1. `todo.md` has drifted into a rename initiative that the active plan and source idea do not authorize. The active plan and idea both still say the initiative is to make `prepare` the shared post-lowering owner and explicitly name `prepare` as the route contract ([plan.md](/workspaces/c4c/plan.md:9), [plan.md](/workspaces/c4c/plan.md:15), [ideas/open/48_prepare_pipeline_rebuild.md](/workspaces/c4c/ideas/open/48_prepare_pipeline_rebuild.md:15), [ideas/open/48_prepare_pipeline_rebuild.md](/workspaces/c4c/ideas/open/48_prepare_pipeline_rebuild.md:37)). But the current packet now frames the goal as making the pipeline read `bir -> prealloc -> mir` and the suggested next packet is to rename the remaining public `prepare` namespace and APIs to `prealloc` ([todo.md](/workspaces/c4c/todo.md:6), [todo.md](/workspaces/c4c/todo.md:11), [todo.md](/workspaces/c4c/todo.md:33)). That is not just scratchpad progress tracking; it proposes a public contract change that would rewrite the meaning of the active runbook.

2. The current code still exposes `prepare` as the public shared route, so `todo.md` is now ahead of the implementation contract and is steering the next packet toward source-idea drift. The backend driver still routes through `prepare_semantic_bir_pipeline(...)` into `c4c::backend::prepare::prepare_semantic_bir_module_with_options(...)` ([src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:38), [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:52), [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:103)). The public header still defines `namespace c4c::backend::prepare`, `PrepareRoute`, and the `prepare_*` entrypoints even though the implementation directory is now `src/backend/prealloc/` and the owning class is `BirPreAlloc` ([src/backend/prealloc/prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp:12), [src/backend/prealloc/prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp:207), [src/backend/prealloc/prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp:246), [src/backend/prealloc/prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp:304)). The route is therefore still substantively a `prepare` route; only the local implementation vocabulary has shifted.

### Medium

3. `plan.md` is now stale enough that it no longer points at the files actually carrying the work, which weakens packet routing and reviewability. Its `Read First`, `Current Targets`, and step targets still point at `src/backend/prepare/*.cpp` and `prepare.cpp` ([plan.md](/workspaces/c4c/plan.md:25), [plan.md](/workspaces/c4c/plan.md:40), [plan.md](/workspaces/c4c/plan.md:83), [plan.md](/workspaces/c4c/plan.md:111)), while the implementation has been moved under `src/backend/prealloc/` and `src/backend/prepare/prepare.cpp` has been deleted in the reviewed range. This is a plan-level documentation drift problem, not yet proof of idea drift, but it is large enough that execution should not continue without a lifecycle refresh.

4. Validation is too narrow for accepting another route-shaping packet without a lifecycle rewrite. The current proof recorded in `todo.md` is a focused backend subset ([todo.md](/workspaces/c4c/todo.md:79)), but the reviewed range includes backend driver routing, directory/API surface changes, and large-scale backend file relocation. That is enough blast radius that the next accepted slice should carry broader backend validation, not only the current narrow subset.

## Judgments

- Plan-alignment judgment: `drifting`
- Idea-alignment judgment: `matches source idea`
- Technical-debt judgment: `action needed`
- Validation sufficiency: `needs broader proof`
- Reviewer recommendation: `rewrite plan/todo before more execution`

## Rationale

The implementation reviewed here still matches the source idea's architectural intent: the shared backend route still enters a prepare-owned contract, legality/liveness/regalloc ownership is still being centralized there, and I did not find testcase-overfit patterns or expectation downgrades in the reviewed packet. The drift is instead at the lifecycle layer. `todo.md` has started narrating the initiative as a `prealloc` rename program, while the code and tests still treat `prepare` as the public contract ([tests/backend/backend_prepare_entry_contract_test.cpp](/workspaces/c4c/tests/backend/backend_prepare_entry_contract_test.cpp:450), [tests/backend/backend_prepare_entry_contract_test.cpp](/workspaces/c4c/tests/backend/backend_prepare_entry_contract_test.cpp:452), [tests/backend/backend_prepare_entry_contract_test.cpp](/workspaces/c4c/tests/backend/backend_prepare_entry_contract_test.cpp:473)).

Because the next suggested packet is to rename the remaining public `prepare` API surface, execution should not continue on the current scratchpad as-is. The lifecycle state needs to be rewritten first so the repo has one coherent answer to whether `prealloc` is merely an internal implementation name under the existing `prepare` idea, or whether the initiative has truly been re-scoped around a renamed public phase. That rewrite belongs in `todo.md` and `plan.md` first; no source-idea rewrite is justified yet from the current code alone.
