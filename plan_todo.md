# Statement Emitter Compression Todo

Status: Active
Source Idea: ideas/open/stmt_emitter_compression_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Compress the statement dispatcher
- Current slice: continue Step 1 by deciding whether the remaining dispatcher endpoints (`CaseStmt`, `CaseRangeStmt`, `DefaultStmt`) should stay as no-op overloads or move behind a final named helper so the statement dispatch surface reads consistently by category

## Completed

- Activated [`plan.md`](/workspaces/c4c/plan.md) from [`ideas/open/stmt_emitter_compression_plan.md`](/workspaces/c4c/ideas/open/stmt_emitter_compression_plan.md)
- Created execution state for the first implementation slice
- Added [`smoke_stmt_control_flow.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_stmt_control_flow.c) to cover `for`, `switch`, `continue`, `break`, `goto`, label, `do-while`, and `while` dispatch paths in compare mode
- Extracted a control-flow helper family behind the existing statement overloads in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)
- Verified targeted compare-mode coverage and full-suite monotonicity (`2241` -> `2242` passed, zero failures)
- Added [`smoke_stmt_non_control_flow.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_stmt_non_control_flow.c) to cover `LocalDecl`, `ExprStmt`, and `InlineAsmStmt` dispatch in compare mode
- Routed non-control-flow statement overloads through `emit_non_control_flow_stmt(...)` helpers in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) without changing the underlying lowering logic
- Verified targeted compare-mode coverage and full-suite monotonicity (`2241/2242` with one known failure -> `2243/2243` with zero failures; regression guard passed with no new failing tests)

## Next Slice

- Continue Step 1 by deciding whether the remaining dispatcher endpoints (`CaseStmt`, `CaseRangeStmt`, `DefaultStmt`) should stay as no-op overloads or move behind a final named helper so the statement dispatch surface reads consistently by category

## Blockers

- None recorded

## Resume Notes

- The statement entrypoint still uses `std::visit`; Step 1 progress now lives in the overload bodies, with control-flow routing through `emit_control_flow_stmt(...)`
- Keep future Step 1 slices additive and local; the new compare case already gives broad dispatcher coverage for control-flow statement kinds
- Step 1 now has named helper families for both control-flow and non-control-flow statement clusters
- The new non-control-flow compare case uses empty inline asm with `"g"` constraints and a memory clobber, which currently exercises declaration, expression-statement, and asm dispatch without target-specific assembly text
