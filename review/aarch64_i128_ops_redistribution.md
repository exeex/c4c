# AArch64 i128 Ops Redistribution Review

Active source idea path: `ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md`

Chosen base commit: `b3cfe7f5b` (`[plan] Activate AArch64 i128 ops shard redistribution plan`)

Base rationale: this commit created the active plan/todo state for the current source idea and is the first lifecycle checkpoint for this redistribution. Later lifecycle commits only recorded packet progress or metadata repair, so they are not the correct review base.

Commits since base: 8

Report path: `review/aarch64_i128_ops_redistribution.md`

## Findings

### Low: close proof metadata is stale relative to the canonical proof log

- `todo.md:57` still records the backend-only proof command with `-R '^backend_'`.
- `todo.md:59` still says 139 selected backend tests passed.
- `test_after.log:6340` records a broader pass: 3167 tests, 0 failures.

This is not an implementation blocker and does not weaken the route. Before plan close, lifecycle state should record the actual final validation command/result or otherwise make clear that the full-suite `test_after.log` supersedes the earlier backend-only packet proof.

## Alignment Review

The route matches the source idea.

- `src/backend/CMakeLists.txt:18` wires in the new compiled owner.
- `src/backend/mir/aarch64/codegen/i128_ops.hpp:20` through `src/backend/mir/aarch64/codegen/i128_ops.hpp:112` exposes a narrow i128-owner API for construction, prepared-record builders, lowering, and printer delegates.
- `src/backend/mir/aarch64/codegen/i128_ops.cpp:111` through `src/backend/mir/aarch64/codegen/i128_ops.cpp:190` owns i128 diagnostics and machine-instruction wrapping that were formerly embedded in dispatch.
- `src/backend/mir/aarch64/codegen/i128_ops.cpp:2395` through `src/backend/mir/aarch64/codegen/i128_ops.cpp:2628` owns i128 pair, div/rem helper-boundary, shift, compare, and copy lowering entry points.
- `src/backend/mir/aarch64/codegen/dispatch.cpp:1563` through `src/backend/mir/aarch64/codegen/dispatch.cpp:1607` now routes to the i128 owner rather than containing i128 lowering bodies.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp:1425` through `src/backend/mir/aarch64/codegen/machine_printer.cpp:1442` delegates i128 spelling to the i128 owner.
- `src/backend/mir/aarch64/codegen/instruction.cpp:107` through `src/backend/mir/aarch64/codegen/instruction.cpp:339` retains only neutral enum/string/status routing for i128 machine node families, while i128-specific constructors and record builders have moved out.
- `src/backend/mir/aarch64/codegen/i128_ops.md` is deleted in the reviewed range.

No testcase-overfit or expectation weakening was found. The reviewed range does not touch tests, supported/unsupported expectations, or route classifications. I also did not find new `print_llvm`, `rendered_contains_*`, `try_emit_minimal_*`, named-case probes, or direct fixed-register accumulator shortcuts. The i128 runtime-helper path continues to consume prepared helper/carrier authority instead of inventing a new fixed-register policy.

The deleted markdown shard contained legacy reference items outside this behavior-preserving redistribution scope, including unary neg/not, multiplication, variable shifts, float/i128 conversion helper calls, fixed accumulator helpers, and generic stack/indirect pair surfaces. `todo.md:23` through `todo.md:28` correctly records those as separate gaps/initiatives rather than silently claiming semantic expansion.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `acceptable`
- Validation sufficiency: `needs broader proof metadata update`
- Reviewer recommendation: `continue current route`

## Residual Blockers Before Plan Close

No implementation blocker found. The only close-readiness item is to reconcile `todo.md` proof metadata with the actual final validation evidence in `test_after.log`.
