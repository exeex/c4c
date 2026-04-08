# Agent Prompt: c4 Plan Execution

Last updated: 2026-03-13

## Mission

Implement features according to [`plan.md`](../plan.md).
Keep test case correctness intact before submitting anything to git.
Prioritize planned feature delivery over opportunistic issue fixing.
Maintain cross-iteration progress in `todo.md` so work can resume cleanly after context resets.
Treat [`plan.md`](../plan.md) as the single active runbook derived from one source idea.
When parallel worker lanes are useful, the mainline agent may delegate bounded
slices to subagents, but mainline remains responsible for lifecycle state,
integration, broad validation, and the final commit unless explicitly handed
off.

## Project Architecture

Compiler pipeline:

1. `src/frontend/preprocessor`: source code -> preprocessed text stream
2. `src/frontend/lexer`: text stream -> token stream
3. `src/frontend/parser`: token stream -> AST
4. `src/frontend/sema`: AST -> validated AST
5. `src/frontend/hir`: validated AST -> HIR
6. `src/codegen/llvm`: HIR -> LLVM IR (`.ll`)

## Non-Negotiable Working Style

1. Follow [`plan.md`](../plan.md) as the primary work queue. Do not default to picking random failing cases unless they block the current plan item.
2. Use a test-driven workflow: add or update test cases before implementing a new feature.
3. Break each plan item into the smallest shippable slice that can be tested.
4. Never guess ABI behavior when a failing case can be compared against Clang IR.
5. Fix one root cause at a time. Prove the change with targeted tests first, then run broader regression checks.
6. Treat Clang/LLVM as the behavioral reference:
   - Preprocessor mode `c4cll --pp-only` should align with `clang -E`
   - Frontend/codegen mode `c4cll` should align with `clang -S -emit-llvm`
7. If Clang cannot compile or run a test case, it is acceptable to mark that case as ignored, but only as a last resort and with an explicit reason.
8. `todo.md` is the execution state for multi-iteration work. Keep it current enough that the next iteration can continue without reconstructing progress from git history or logs.

## State Tracking

1. Read [`plan.md`](../plan.md) first.
2. If `todo.md` does not exist, create it from `plan.md` before making code changes.
3. If `todo.md` already exists, treat it as the current execution state and continue from it.
4. Confirm that [`plan.md`](../plan.md) identifies its `Source Idea` near the top before implementation.
5. Confirm that [`todo.md`](../todo.md), when present, matches the same source idea.
6. `todo.md` should track at least:
   - the current active plan item
   - completed items
   - the next intended slice
   - blockers, if any
   - short notes needed to resume in the next iteration
7. Update `todo.md` whenever the active item changes, when a meaningful sub-step is completed, and before every commit.

## Parallel Delegation

1. Mainline may spawn subagents for narrow, non-overlapping work packets.
2. Before delegating, define explicit ownership boundaries and local validation
   commands for each worker lane.
3. Use worker packet files such as `todoA.md` through `todoD.md` only as
   delegation aids; they do not replace [`todo.md`](../todo.md) as the canonical
   execution state.
4. When creating a worker lane, direct the subagent to read
   [`SUBAGENT.md`](../prompts/SUBAGENT.md).
5. Mainline keeps responsibility for:
   - [`plan.md`](../plan.md)
   - [`todo.md`](../todo.md)
   - cross-lane conflict resolution
   - broad validation
   - final commit and lifecycle transitions
6. Subagents should normally:
   - avoid broad `ctest` runs
   - avoid editing planning files
   - validate only owned objects or narrow owned tests

## Work Selection

1. Start from `todo.md` if it exists; otherwise create it from [`plan.md`](../plan.md) and start there.
2. Choose the highest-priority incomplete item recorded in `todo.md`.
3. Do not switch into issue-fixing mode unless:
   - the issue blocks the current plan item
   - the issue is directly uncovered by the current implementation work
   - the user explicitly asks for issue triage or regression fixing
4. If execution uncovers a genuinely separate initiative, write it into `ideas/open/*.md` and request a lifecycle transition instead of silently mutating the active plan.

## Required Development Flow

Given a plan item or plan-blocking failure:

0. Sync planning state first:
   - read [`plan.md`](../plan.md)
   - create `todo.md` if it does not exist
   - if `todo.md` exists, read it and continue from the recorded active item
   - update `todo.md` to mark the exact target for this iteration

1. Define the immediate target:
   - identify the exact behavior, syntax, or subsystem from `plan.md` being implemented
   - define the narrowest testable slice for this iteration
   - record that slice in `todo.md`

2. Record the current full-suite baseline:

```bash
cmake -S . -B build
cmake --build build -j8
ctest --test-dir build -j --output-on-failure > test_before.log
```

3. Add or update the narrowest validating test before implementation.

   Timeout policy:
   - Default per-test timeout: `30s`
   - Any test runtime longer than `30s` is suspicious unless explicitly documented

4. Capture our output and classify the affected area:
   - `PREPROCESSOR` -> `src/frontend/preprocessor/`
   - `FRONTEND_FAIL` -> `src/frontend/parser/`, `src/frontend/sema/`, or `src/frontend/hir/`
   - `BACKEND_FAIL` or `RUNTIME_MISMATCH` -> `src/codegen/llvm/`
   - Generate HIR: `build/c4cll --dump-hir /path/to/case.c`
   - Generate LLVM IR: `build/c4cll /path/to/case.c`

5. Compare against the reference behavior when applicable:
   - For preprocessing, compare against `clang -E`
   - For frontend/codegen, capture Clang IR on the same target triple:
     - `clang -S -emit-llvm -O0 <case>.c -o /tmp/clang.ll`
   - Use the host triple by default
   - Only add an explicit target such as `-target aarch64-unknown-linux-gnu` when the test is target-specific

6. Implement the smallest change that makes the target behavior correct.

7. Re-run:
   - the targeted test
   - nearby tests in the same subsystem
   - the full `ctest -j` suite before handoff

```bash
rm -rf build
cmake -S . -B build
cmake --build build -j8
ctest --test-dir build -j --output-on-failure > test_after.log
```

8. Verify the result:
   - compare `test_before.log` and `test_after.log`
   - full-suite results must be monotonic
   - no previously passing test may become failing
   - total passed tests in `test_after.log` must be greater than or equal to `test_before.log`

`=` is acceptable for feature work or refactors that preserve the current pass count.

9. Update planning state before handoff or commit:
   - mark completed sub-steps in `todo.md`
   - record the next intended slice in `todo.md`
   - record blockers or deferred work in `todo.md`
   - keep `todo.md` aligned with the actual code and tests in the tree

10. Optionally update `known_issues.md` if new blockers or deferred failures are discovered.

11. Commit one plan item slice per commit. Before committing, update `todo.md` so the repo state clearly reflects current progress. Do not bundle unrelated fixes together.
**important** this is not an interative conversation, you must git commit after finished.
