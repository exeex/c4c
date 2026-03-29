# x86 Regalloc And Peephole Enablement Todo

Status: Active
Source Idea: ideas/open/16_backend_x86_regalloc_peephole_enablement_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 4: decide whether the bounded x86 conditional-return scaffold should stop after the signed predicate quartet (`slt|sle|sgt|sge`) or whether to return to one more narrow direct-call regalloc-sensitive shape

## Planned Queue

- [ ] Step 4: otherwise return to the direct-call scaffold for one more bounded call shape that still exercises shared regalloc state

## Completed

- [x] Activated `ideas/open/16_backend_x86_regalloc_peephole_enablement_plan.md` into the active runbook
- [x] Step 1: selected `make_typed_call_crossing_direct_call_module` as the first x86 proof case and recorded the expected shared callee-saved handoff plus bounded cleanup target before implementation
- [x] Step 2: wired the x86 emit scaffold for the typed call-crossing direct-call slice so shared regalloc now chooses the surviving callee-saved register, saves/restores it in `main`, and reuses it across `call add_one`
- [x] Step 3: enabled one bounded post-codegen x86 cleanup on that same slice by removing redundant backend-owned self-moves from the emitted assembly text
- [x] Step 4: rebuilt, ran `./backend_lir_adapter_tests`, and recorded monotonic full-suite validation with `ctest --test-dir build -j --output-on-failure` (`29` failing tests before and after, `2338` total)
- [x] Step 4: switched to the bounded x86 signed-less-than conditional-return proof case, added the failing scaffold test first, and lowered that slice to direct x86 assembly without widening into general branch lowering
- [x] Step 4: rebuilt, reran `backend_lir_adapter_tests`, and recorded monotonic full-suite validation with `ctest --test-dir build -j --output-on-failure` plus the regression guard (`29 -> 28` failing tests, `2309 -> 2310` passed, `2338` total)
- [x] Step 4: extended the x86 conditional-return scaffold to the adjacent signed less-or-equal (`sle`) predicate with a failing proof test first, then rebuilt, reran `./backend_lir_adapter_tests`, and recorded monotonic full-suite validation plus the regression guard (`29 -> 27` failing tests, `2309 -> 2311` passed, `2338` total)
- [x] Step 4: extended the x86 conditional-return scaffold to the adjacent signed greater-than (`sgt`) predicate with a failing proof test first, then rebuilt and reran `./build/backend_lir_adapter_tests`
- [x] Step 4: extended the x86 conditional-return scaffold to the adjacent signed greater-or-equal (`sge`) predicate with a failing proof test first, rebuilt `backend_lir_adapter_tests` plus `c4cll`, reran the bounded adapter/runtime proofs, and recorded monotonic full-suite validation plus the regression guard (`27 -> 26` failing tests, `2311 -> 2312` passed, `2338` total)

## Notes

- Initial activation chose idea `16_backend_x86_regalloc_peephole_enablement_plan.md` as the next bounded follow-on slice after the closed x86 bring-up and shared regalloc/peephole port plans.
- Keep this run focused on one callee-saved-sensitive or redundant-text x86 asm case. Do not widen into local/global addressing or built-in assembler/linker work.
- Selected proof case: the existing typed direct-call helper slice already used on AArch64 (`make_typed_call_crossing_direct_call_module`). Expected x86 seam: one shared regalloc-assigned callee-saved register should survive the `call add_one` path in `main`, and the post-codegen text cleanup should remove one backend-owned redundant move on that same slice.
- Execution uncovered two pre-existing build blockers outside the active emit slice but on the same x86 backend path: `src/backend/x86/assembler/mod.hpp` was missing an `AsmItem` forward declaration, and `src/backend/x86/linker/mod.hpp` was missing the staged `ContractSurface` marker used by backend contract tests.
- The current x86 assembler entrypoint still stages raw text to the requested output path instead of producing a real object file. That was kept as a bounded unblocker for this run rather than widening into assembler implementation work.
- The new x86 compare-and-branch scaffold currently claims only the minimal signed-less-than conditional-return shape (`icmp slt` + `zext` + `icmp ne 0` + two direct return blocks). Adjacent predicates remain separate follow-on work.
- The x86 compare-and-branch scaffold now covers the minimal signed-less-than and signed-less-or-equal conditional-return shapes (`icmp slt|sle` + `zext` + `icmp ne 0` + two direct return blocks). Other predicates remain separate follow-on work.
- The x86 compare-and-branch scaffold now covers the minimal signed-less-than, signed-less-or-equal, and signed-greater-than shapes (`icmp slt|sle|sgt` + `zext` + `icmp ne 0` + two direct return blocks). `sge` and non-signed predicates remain separate follow-on work.
- Clean rebuild validation after the `sgt` slice kept the aggregate full-suite count flat at `27` failing and `2311` passing out of `2338`, fixed `backend_runtime_branch_if_gt`, and surfaced an unrelated new `llvm_gcc_c_torture_src_struct_ret_1_c` failure where `clang -x ir` rejects the emitted IR for a struct-return function-pointer call. Keep that as a separate blocker or follow-on idea, not as part of this x86 conditional-return plan.
- The x86 compare-and-branch scaffold now covers the bounded signed predicate quartet (`icmp slt|sle|sgt|sge` + `zext` + `icmp ne 0` + two direct return blocks). `eq`, `ne`, and unsigned predicates remain separate follow-on work.
- Clean rebuild validation after the `sge` slice improved the aggregate full-suite count from `27` failing / `2311` passing to `26` failing / `2312` passing out of `2338` by fixing `backend_runtime_branch_if_ge`. The separate `llvm_gcc_c_torture_src_struct_ret_1_c` inventory idea remains open and out of scope for this plan.

## Next Intended Slice

Decide whether the x86 conditional-return scaffold should stop after the bounded signed predicate quartet or whether the next narrow proof should return to the direct-call scaffold for one more shared-regalloc-sensitive save/restore shape.
