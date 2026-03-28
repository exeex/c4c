# Statement Emitter Compression Todo

Status: Active
Source Idea: ideas/open/stmt_emitter_compression_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Extract Terminator and Block Lifecycle Helpers
- Current slice: inspect the next remaining Step 2 block-lifecycle repetition after the `emit_fallthrough_lbl(...)` reuse pass, likely another small local join/open helper in `va_arg` lowering or another uncovered successor-opening site inside [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)

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
- Extended [`smoke_stmt_control_flow.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_stmt_control_flow.c) with an open-block label-fallthrough path to pin the first Step 2 block-lifecycle slice in compare mode
- Extracted `set_terminator_if_open(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and routed simple branch/condbr/ret/switch terminator helpers through it so label fallthrough and simple terminator sites share the same open-block contract
- Verified the targeted compare-mode cases after the helper extraction and passed the full-suite regression guard with stable results (`2243` -> `2243` passed, zero failures)
- Extended [`smoke_stmt_control_flow.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_stmt_control_flow.c) with a second case-range switch that forces a false-first-range / true-second-range chain before `tail`
- Extracted `emit_fallthrough_lbl(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and reused it for switch range-chain successor blocks plus user-label opening so both sites share the same open-block fallthrough contract
- Verified the targeted compare-mode case, the full `compare_case` label, and the clean full-suite regression guard (`2243` -> `2243` passed, zero failures)
- Added [`smoke_expr_branch_lifecycle.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c) to cover short-circuit logical evaluation and ternary branch opening in compare mode
- Extracted `emit_condbr_and_open_lbl(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and reused it across logical expression lowering, ternary lowering, and AArch64 `va_arg` branch setup so these sites share the same conditional-branch successor-opening contract
- Verified targeted compare-mode coverage plus full-suite regression guard monotonicity (`2243` -> `2244` passed, zero failures)
- Extended [`smoke_expr_branch_lifecycle.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c) with nested logical-in-ternary branches so the compare-mode case now traverses additional immediate join/open successors
- Reused `emit_fallthrough_lbl(...)` across immediate branch-plus-open sites in logical, ternary, `for`, and `do-while` lowering inside [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp), removing the remaining manual same-target branch/open pairs in those paths
- Verified the targeted compare-mode cases, the full `compare_case` label, and the clean full-suite regression guard with stable results (`2244` -> `2244` passed, zero failures)

## Next Slice

- Continue Step 2 by looking for another small local block-lifecycle helper around the remaining join/open-label scaffolding in `va_arg` lowering or another uncovered successor-opening path without widening the refactor beyond [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)

## Blockers

- None recorded

## Resume Notes

- The statement entrypoint still uses `std::visit`; Step 1 progress now lives in the overload bodies, with control-flow routing through `emit_control_flow_stmt(...)`
- Keep future Step 1 slices additive and local; the new compare case already gives broad dispatcher coverage for control-flow statement kinds
- Step 1 now has named helper families for both control-flow and non-control-flow statement clusters
- The new non-control-flow compare case uses empty inline asm with `"g"` constraints and a memory clobber, which currently exercises declaration, expression-statement, and asm dispatch without target-specific assembly text
- This iteration targets the last uncategorized statement overloads only; no switch semantics should change, because case/default markers remain no-op statement emitters and are consumed by switch lowering
- Step 1 is complete: the dispatcher surface is now split into non-control-flow, control-flow, and switch-label helper families under the existing `std::visit` entrypoint
- Current Step 2 slice is intentionally narrow: one helper around open-block branch placement, validated by the existing compare-mode control-flow case extended with a label-fallthrough path
- The current helper lives in the anonymous namespace and does not change public emitter structure; the next Step 2 slice should keep that same local-only extraction style
- This iteration now targets the shared fallthrough-open-block pattern used by label emission and switch range-chain successor blocks; keep the extraction local to the anonymous namespace and validate it with compare-mode coverage that traverses more than one case-range hop
- `emit_fallthrough_lbl(...)` is now the local wrapper for "terminate current block if still open, then open the named successor block"; remaining Step 2 opportunities should build on that contract instead of reintroducing manual branch-plus-label sequences
- This iteration is focused on the sibling lifecycle pattern where a conditional branch is emitted and code generation immediately continues in the false/next successor block; keep the helper local and use compare-mode coverage that exercises short-circuit or ternary lowering
- `emit_condbr_and_open_lbl(...)` now owns the "place a conditional terminator, then continue emission in one chosen successor block" contract; future Step 2 slices should compose with it instead of restating manual `emit_term_condbr(...)` + `emit_lbl(...)` pairs
- This iteration targets the sibling "branch directly to the block we open next" pattern; prefer reusing `emit_fallthrough_lbl(...)` rather than introducing another near-duplicate helper when the target label is the same
