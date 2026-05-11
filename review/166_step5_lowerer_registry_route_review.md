# Step 5 Lowerer Registry Route Review

Active source idea: `ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md`

Chosen base commit: `30c30bdb4 [plan] Activate HIR rendered registry mirror audit`

Why this base: this commit activated the current source idea by creating the active `plan.md` and `todo.md`. The later `fd246889d [todo_only] Inventory rendered HIR registries` commit is execution-state inventory for the same source idea, not a source-idea reset. The previous `review/166_compile_time_registry_fencing_route_review.md` also used `30c30bdb4` and found no later reviewer checkpoint that should reset the active idea base.

Commit count since base: 14

Reviewed scope:
- `ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md`
- `plan.md`
- `todo.md`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/impl/expr/object.cpp`
- `src/frontend/hir/impl/stmt/stmt.cpp`
- `tests/frontend/frontend_hir_tests.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

## Findings

### Medium: Step 5 should stay packetized before any lifecycle handoff

`plan.md:216` through `plan.md:223` lists several lowerer registry families, including constructor/destructor maps and overload maps. The current `todo.md:19` through `todo.md:23` says the supervisor can either choose the next remaining lowerer registry packet or perform lifecycle handoff after the constructor/destructor slice.

The implementation reviewed here covers static-member decls, static-member consts, struct methods, constructor lookups, and member destructor routing, but I do not see evidence in this review scope that lowerer overload maps have been retired, fenced, or explicitly retained with owner/limitation/removal-condition notes. That is not route drift, because Step 5 explicitly says to process one registry family at a time. It does mean Step 5 should not be treated as closure-ready from the constructor/destructor slice alone.

Recommendation: do not rewrite the source idea or reset the route. Keep Step 5 active and narrow the next packet in `todo.md` to the remaining lowerer family, especially overload maps or any other lowerer registry not yet documented against `plan.md:232` through `plan.md:235`.

### Low: Recorded proof references a missing canonical log artifact

`todo.md:35` through `todo.md:40` records a passing delegated proof command that writes `test_after.log`, but no root-level `test_after.log` is present in the worktree during this review. I did not rerun tests because the reviewer role should not mutate validation state.

This does not undermine the route shape or show testcase-overfit. It does leave acceptance proof less inspectable than the canonical validation discipline expects. Before accepting or rolling the slice forward, the supervisor should either regenerate the exact proof log or normalize validation state according to the supervisor/regression-guard workflow.

## Route Review

The implementation still matches the source idea. The source idea requires rendered maps to stop acting as ordinary semantic authority when complete structured keys exist, while preserving explicit no-metadata compatibility (`ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md:45` through `ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md:49`, reject signal at `ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md:76` through `ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md:77`).

The lowerer helper added at `src/frontend/hir/hir_types.cpp:802` through `src/frontend/hir/hir_types.cpp:873` is aligned with that rule. It resolves `record_def` and namespace/TextId owners through structured module owner keys and returns `nullopt` on complete structured misses. Rendered compatibility remains only after metadata is absent or insufficient.

The lowerer static-member and method lookups also preserve the intended fence:
- `src/frontend/hir/hir_types.cpp:2007` through `src/frontend/hir/hir_types.cpp:2034` rejects rendered static-member decl fallback after a complete owner-key miss, while still walking structured base tags.
- `src/frontend/hir/hir_types.cpp:2091` through `src/frontend/hir/hir_types.cpp:2235` does the same for static constexpr values while preserving structured base and selected-template-pattern paths.
- `src/frontend/hir/hir_types.cpp:1034` through `src/frontend/hir/hir_types.cpp:1184` disables rendered method fallback once a complete owner method key is available and missed.

The constructor/destructor slice follows the same route rather than adding a named-case shortcut:
- `src/frontend/hir/impl/expr/object.cpp:92` through `src/frontend/hir/impl/expr/object.cpp:159` resolves direct constructor owner tags through the shared lowerer registry helper, so a `record_def` miss fails closed instead of using the rendered callee name.
- `src/frontend/hir/impl/expr/object.cpp:1031` through `src/frontend/hir/impl/expr/object.cpp:1072` fences `new` constructor lookup through the same owner resolver.
- `src/frontend/hir/impl/stmt/stmt.cpp:94` through `src/frontend/hir/impl/stmt/stmt.cpp:104` and `src/frontend/hir/impl/stmt/stmt.cpp:210` through `src/frontend/hir/impl/stmt/stmt.cpp:227` use that resolver for member destructor presence and emission.
- `src/frontend/hir/impl/stmt/stmt.cpp:963` through `src/frontend/hir/impl/stmt/stmt.cpp:970` applies the same owner resolution to constructor-initializer field constructor lookup.

The tests are semantic and cover both sides of the boundary. Examples include stale rendered rejection for static-member const lookup at `tests/frontend/frontend_hir_tests.cpp:3153`, static-member decl lookup at `tests/frontend/frontend_hir_tests.cpp:3238`, struct method lookup at `tests/frontend/frontend_hir_tests.cpp:3672`, direct constructor lookup at `tests/frontend/frontend_hir_tests.cpp:4715`, and member destructor lookup at `tests/frontend/frontend_hir_tests.cpp:5819`. Compatibility coverage remains present, for example direct constructor no-owner fallback at `tests/frontend/frontend_hir_tests.cpp:4772` and member destructor no-owner fallback at `tests/frontend/frontend_hir_tests.cpp:5864`.

I do not see testcase-overfit. The code introduces a shared semantic owner resolver and applies it across multiple lowerer call sites; the tests construct stale rendered collisions to prove fail-closed behavior and explicit no-metadata compatibility rather than weakening expectations or matching one named fixture.

## Judgments

Idea alignment: `matches source idea`

Runbook transcription: `plan matches idea`

Route alignment: `on track`

Technical debt: `watch`

Validation sufficiency: `needs broader proof` before Step 6 closure; focused proof is plausible for the current constructor/destructor packet but the recorded `test_after.log` artifact is missing from the worktree.

Reviewer recommendation: `narrow next packet`

Step 5 does not need to be split or replanned before more execution. Continue the current route, but keep the next packet explicit and bounded to the remaining lowerer registry family rather than hand off to lifecycle closure from this constructor/destructor slice alone.
