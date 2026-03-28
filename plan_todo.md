# Statement Emitter Compression Todo

Status: Active
Source Idea: ideas/open/stmt_emitter_compression_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Extract Terminator and Block Lifecycle Helpers
- Current slice: identify the first repeated unterminated-block guard around label and branch emission, then extract the smallest reusable helper without changing lowering behavior

## Completed

- Activated [`plan.md`](/workspaces/c4c/plan.md) from [`ideas/open/stmt_emitter_compression_plan.md`](/workspaces/c4c/ideas/open/stmt_emitter_compression_plan.md)
- Created execution state for the first implementation slice
- Added [`smoke_stmt_control_flow.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_stmt_control_flow.c) to cover `for`, `switch`, `continue`, `break`, `goto`, label, `do-while`, and `while` dispatch paths in compare mode
- Extracted a control-flow helper family behind the existing statement overloads in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)
- Verified targeted compare-mode coverage and full-suite monotonicity (`2241` -> `2242` passed, zero failures)
- Added [`smoke_stmt_non_control_flow.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_stmt_non_control_flow.c) to cover `LocalDecl`, `ExprStmt`, and `InlineAsmStmt` dispatch in compare mode
- Routed non-control-flow statement overloads through `emit_non_control_flow_stmt(...)` helpers in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) without changing the underlying lowering logic
- Verified targeted compare-mode coverage and full-suite monotonicity (`2241/2242` with one known failure -> `2243/2243` with zero failures; regression guard passed with no new failing tests)
- Extended [`smoke_stmt_control_flow.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_stmt_control_flow.c) to cover `case ... ...` and the default switch-label path in compare mode
- Routed `CaseStmt`, `CaseRangeStmt`, and `DefaultStmt` through `emit_switch_label_stmt(...)` helpers in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) so the remaining statement dispatcher endpoints now live behind named helper families
- Closed Step 1 after targeted compare-mode coverage plus full-suite regression guard stability (`2243` -> `2243` passed, zero failures)

## Next Slice

- Start Step 2 by extracting one helper for the repeated "block still open" contract shared by label creation and simple branch/terminator emission sites

## Blockers

- None recorded

## Resume Notes

- The statement entrypoint still uses `std::visit`; Step 1 progress now lives in the overload bodies, with control-flow routing through `emit_control_flow_stmt(...)`
- Keep future Step 1 slices additive and local; the new compare case already gives broad dispatcher coverage for control-flow statement kinds
- Step 1 now has named helper families for both control-flow and non-control-flow statement clusters
- The new non-control-flow compare case uses empty inline asm with `"g"` constraints and a memory clobber, which currently exercises declaration, expression-statement, and asm dispatch without target-specific assembly text
- This iteration targets the last uncategorized statement overloads only; no switch semantics should change, because case/default markers remain no-op statement emitters and are consumed by switch lowering
- Step 1 is complete: the dispatcher surface is now split into non-control-flow, control-flow, and switch-label helper families under the existing `std::visit` entrypoint
