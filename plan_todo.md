# Statement Emitter Compression Todo

Status: Active
Source Idea: ideas/open/stmt_emitter_compression_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Extract Aggregate and Address-Lowering Helpers
- Current slice: rescan [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) for the next duplicated aggregate/address-formation path after landing the shared `MemberExpr` base-type/base-pointer helpers

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
- Added [`smoke_vaarg_branch_lifecycle.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_vaarg_branch_lifecycle.c) to force AArch64 `va_arg` GP and FP lowering through both register and stack join paths in compare mode
- Reused `emit_fallthrough_lbl(...)` for the remaining AArch64 `va_arg` GP/FP stack-to-join block openings inside [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp), removing the last manual `emit_term_br(join_lbl)` + `emit_lbl(join_lbl)` pairs in those helpers
- Verified the new `compare_smoke_vaarg_branch_lifecycle` test, the full `compare_case` label, and the full-suite regression guard (`2243/2244` with one failing baseline case -> `2245/2245` with zero failures; guard passed with no new failing tests)
- Extended [`smoke_expr_branch_lifecycle.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c) with nested ternary and logical branches that force the remaining sibling-successor handoff pattern through compare mode
- Extracted `emit_br_and_open_lbl(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and reused it across logical short-circuit, ternary lowering, and the remaining AArch64 `va_arg` reg-to-stack handoff sites so those paths share one explicit sibling-block transition contract
- Verified the targeted compare-mode cases plus the clean full-suite regression guard with stable results (`2245` -> `2245` passed, zero failures)
- Extended [`smoke_expr_branch_lifecycle.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c) with one more nested ternary/logical branch to keep the conditional-successor opening path pinned in compare mode during the next Step 2 extraction
- Extracted `open_lbl(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and reused it across `emit_lbl(...)` plus `emit_condbr_and_open_lbl(...)` so direct label opens and conditional successor opens share one local block-allocation contract
- Verified the targeted compare-mode cases and the clean full-suite regression guard with stable results (`2245` -> `2245` passed, zero failures; non-decreasing pass-count guard accepted the refactor slice)
- Added [`smoke_switch_range_chain_lifecycle.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_switch_range_chain_lifecycle.c) to pin chained case-range successor opening in compare mode across false-first, mid-chain match, and default paths
- Extracted `emit_condbr_and_fallthrough_lbl(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and reused it for case-range switch chaining so the remaining conditional-branch-plus-fallthrough-open pattern now shares one local helper
- Verified `compare_smoke_stmt_control_flow` plus `compare_smoke_switch_range_chain_lifecycle`, then passed the clean full-suite regression guard (`2244/2245` before with one known failure -> `2246/2246` after with zero failures; no new failing tests)
- Added [`smoke_for_latch_lifecycle.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_for_latch_lifecycle.c) to pin `for` condition evaluation, `continue` routing through `for.latch.*`, and the false-exit path in compare mode
- Extracted `emit_condbr_and_open_sibling_lbl(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and reused it for `ForStmt` condition lowering so the remaining condition-to-sibling-open loop path now shares a named local lifecycle helper
- Verified `compare_smoke_for_latch_lifecycle`, `compare_smoke_stmt_control_flow`, the full `compare_case` label, and the clean full-suite regression guard (`2246` -> `2247` passed, zero failures)
- Extended [`smoke_for_latch_lifecycle.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_for_latch_lifecycle.c) with a no-condition `for (;; update)` loop that exercises the remaining body-branch / latch-open handoff via `continue`, update, and `break`
- Reused `emit_br_and_open_lbl(...)` for the remaining `ForStmt` no-condition handoff in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp), removing the last repeated Step 2 sibling-open lifecycle pattern
- Closed Step 2 after targeted compare-mode coverage, a full rebuild, and the regression guard passed with stable results (`2247` -> `2247` passed, zero failures; no new failing tests)
- Added [`smoke_aggregate_access.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_aggregate_access.c) to cover scalar member loads, array indexing, array-field decay through `.` and `->`, and pointer-to-array style aggregate access in compare mode
- Extracted `emit_rval_from_access_ptr(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) so `IndexExpr` and `MemberExpr` now share one local "access pointer -> decay or load" helper, while member bitfields still keep their dedicated load path
- Fixed array-field type reconstruction in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and [`const_init_emitter.cpp`](/workspaces/c4c/src/codegen/lir/const_init_emitter.cpp) so declared array dimensions override inherited pointer-to-array metadata and preserve multi-dimensional fields
- Fixed pointer-to-array access lowering in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) by distinguishing outer array dimensions from inner pointer-to-array metadata during indexed lvalue/rvalue lowering
- Verified targeted compare-mode coverage, the previously uncovered torture regressions (`20020402-3`, `20071018-1`, `950426-1`, `pr41463`, `pr69691`, `strlen-4`), and the clean full-suite regression guard (`2247` -> `2248` passed, zero failures)
- Extended [`smoke_aggregate_access.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_aggregate_access.c) with a direct pointer-to-array indexing path (`pvalues[0][0]`, `pvalues[0][2]`) so compare mode keeps the indexed element-address rule pinned for array-object and pointer-to-array bases
- Extracted `drop_one_indexed_element_type(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and reused it across `IndexExpr` lvalue lowering, `IndexExpr` type resolution, and pointer-arithmetic GEP element selection so those address-formation paths now share one local indexed-element contract
- Verified `compare_smoke_aggregate_access`, the full `compare_case` label, and the clean full-suite regression guard with stable results (`2248` -> `2248` passed, zero failures; no new failing tests)
- Extended [`smoke_aggregate_access.c`](/workspaces/c4c/tests/c/internal/compare_case/smoke_aggregate_access.c) with typedef-carried `->` access and struct-rvalue `.` access so compare mode now pins both the `MemberExpr` base-pointer path and the aggregate-rvalue materialization path
- Extracted `resolve_member_base_type(...)` and `emit_member_base_ptr(...)` in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp), reusing the shared base-resolution contract across `emit_member_lval(...)` and `resolve_payload_type(MemberExpr)` without changing downstream `emit_member_gep(...)` or access-load behavior
- Verified `compare_smoke_aggregate_access`, the full `compare_case` label, Clang IR shape for the new aggregate-rvalue/member-base paths, and the clean full-suite regression guard (`2248` -> `2248` passed, zero failures; no new failing tests)

## Next Slice

- Re-scan Step 3 in [`stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) for the next shared address-formation seam, most likely around remaining aggregate/member access work immediately upstream or downstream of `emit_member_gep(...)`

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
- The next concrete Step 2 candidate is the AArch64 `va_arg` GP/FP join path, where both branches still end with manual `emit_term_br(join_lbl)` plus `emit_lbl(join_lbl)` instead of the existing fallthrough-open helper
- The AArch64 `va_arg` join path now composes with `emit_condbr_and_open_lbl(...)` and `emit_fallthrough_lbl(...)`; the next Step 2 pass should look for other local block-opening sites that still build labels manually after placing a terminator
- The current Step 2 slice is the repeated "finish one predecessor, branch to a join/end label, then immediately continue emitting the sibling block" shape that appears in logical, ternary, and the remaining `va_arg` reg-to-stack handoff paths
- `emit_br_and_open_lbl(...)` now owns the sibling-handoff shape where one predecessor closes by branching to a join/end label and codegen immediately continues in a different successor block; future Step 2 slices should reuse it instead of restating `emit_term_br(...)` plus `emit_lbl(...)`
- The next narrow Step 2 extraction is the duplicated block-allocation/opening sequence currently spelled separately in `emit_lbl(...)` and `emit_condbr_and_open_lbl(...)`; keep it local and validate with compare-mode coverage that still crosses both surfaces
- `open_lbl(...)` now owns the raw block-allocation/opening sequence; future Step 2 slices should build on it instead of spelling out `LirBlock` allocation and `current_block_idx` updates in additional local helpers
- The current slice targets the remaining "emit terminator for a branch decision, then continue in the chosen successor/open block" pattern still present in loop and switch-range lowering; validate it with compare-mode coverage that traverses the false/range-chain continuation path
- `emit_condbr_and_fallthrough_lbl(...)` now owns the "emit a conditional terminator, then continue in the false/next successor block" pattern used by case-range switch chaining; future Step 2 scans should treat any remaining manual `emit_term_condbr(...)` + immediate false/open continuation as the next extraction candidate
- The current Step 2 target is narrower than that previous scan result: `ForStmt` still spells the condition terminator and immediate latch-block opening separately, so the next helper should encode that sibling-open contract without changing loop semantics
- `emit_condbr_and_open_sibling_lbl(...)` now owns the `for` condition-to-latch sibling-open contract; the next Step 2 pass should decide whether any other repeated lifecycle shape remains or whether Step 2 is effectively exhausted
- The final Step 2 scan found no further repeated block-lifecycle helper opportunity beyond the `for` no-condition handoff; remaining branch/terminator sites are single-purpose control-flow emitters, so execution moves to Step 3 next
- The first Step 3 slice is intentionally narrow and behavior-preserving: unify only the post-address access behavior that decides between array decay, bitfield load, scalar load, or empty-void result after `IndexExpr`/`MemberExpr` already computed an access pointer
- Step 3 uncovered a real clean-build blocker in pointer-to-array aggregate access (`block[2]`, `paa`, `pa0/pa1`, and typedef-carried multi-dimensional arrays): future address-lowering extractions need to watch `outer_array_rank(...)`, `is_ptr_to_array`, and `inner_rank` carefully so array objects and pointer-to-array values are not conflated
- `drop_one_indexed_element_type(...)` now owns the "consume one indexing step from array/pointer/pointer-to-array type metadata" rule; the next Step 3 pass should look upstream at duplicated base-pointer acquisition or member-base materialization rather than re-spelling type-drop logic again
- `resolve_member_base_type(...)` now owns the canonical `MemberExpr` base-type lookup for both field-type resolution and lvalue lowering, including typedef-unwrapping before struct/union tag checks
- `emit_member_base_ptr(...)` now owns the `MemberExpr` base-pointer/materialization split between `->`, direct aggregate lvalues, and struct-rvalue temporary slots; the next Step 3 scan should look beyond member-base setup rather than reintroducing inline `MemberExpr` base handling
