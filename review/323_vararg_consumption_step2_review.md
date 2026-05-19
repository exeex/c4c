# Idea 323 Step 2 Review

Active source idea path: `ideas/open/323_aarch64_vararg_consumption_source_progression.md`

Chosen review base: `628f37e95` (`[plan+idea] Close idea 322 and activate vararg consumption source progression`). This is the commit that created and activated idea 323, rewrote `plan.md`, and reset `todo.md` for the current source idea. `b1967bd5f` is the committed Step 1 localization inside the same idea, so it is useful context but not the active-idea activation checkpoint.

Commits since base: 1 committed change to `HEAD`, plus the current uncommitted Step 2 implementation under review.

## Findings

No blocking findings.

Medium watch: HFA aggregate detection is structural and somewhat implicit, but not testcase-shaped.

- Reference: `src/backend/prealloc/variadic_entry_plans.cpp:118`
- Reference: `src/backend/prealloc/variadic_entry_plans.cpp:568`

`infer_aapcs64_hfa_va_arg_shape()` identifies HFA/floating aggregate `va_arg` consumers by scanning later `LoadLocalInst` uses of the sret aggregate payload slot and requiring homogeneous F32/F64/F128 lanes, contiguous offsets, no more than four lanes, and exact payload-size coverage. That is broader than `00204.c`/`myprintf`/one-register/one-offset matching, and the resulting access plan is expressed as source/progression facts (`fp_register_save_area`, `fp_offset`, lane count/size) rather than a named case shortcut. The risk is maintainability: this uses post-helper local-load shape as the HFA classifier instead of a first-class aggregate ABI classification fact, so future BIR shape changes could make the detection miss valid HFA consumers or conservatively fall back to overflow. This is a watch item, not a route blocker for this slice.

Low watch: Step 2 extends `va_start` lowering to populate the register-save area, which is adjacent to the idea's main `va_arg` consumer scope.

- Reference: `src/backend/mir/aarch64/codegen/variadic.cpp:529`
- Reference: `src/backend/mir/aarch64/codegen/variadic.cpp:589`

This is still aligned because HFA/floating `va_arg` source selection cannot work unless the initialized `va_list`'s GP/FP register-save slots contain the unnamed incoming register values. The patch does not reopen idea 322's destination publication repair; it preserves the destination materialization and adds source data publication needed by idea 323.

## Route And Overfit Judgment

The patch repairs a general AArch64 HFA/floating aggregate `va_arg` source/progression capability. The prepared plan now carries reusable source/progression metadata (`register_save_lanes`, lane size, overflow fallback, source/progression fields), and the printer emits a generic register-save-area branch with `FpOffset` progression and overflow fallback.

I did not find testcase-overfit signals: no `00204.c`, `stdarg`, `myprintf`, argument-index, register, stack-slot, offset, or emitted-line special case appears in the implementation. Labels generated in `src/backend/mir/aarch64/codegen/variadic.cpp:830` use function/block/instruction identity for assembler label uniqueness, not for semantic dispatch.

## Validation

The focused proof recorded in `test_after.log` is sufficient for accepting this implementation slice, but not for closing idea 323. It includes a fresh build, prior-owner guardrails, focused prepared/MIR dump coverage, machine-printer/target-record/dispatch tests, and the `00204.c` representative. Result: 10/11 passed; only `c_testsuite_aarch64_backend_src_00204_c` still fails with a runtime segfault. `git diff --check` also passes.

Because the external representative still fails, the next packet should localize the new first bad fact before any lifecycle close. Broader validation can wait until Step 4 or a milestone, unless the supervisor wants extra confidence for the widened shared variadic surface.

## Judgments

- Idea alignment: matches source idea
- Runbook transcription: plan matches idea
- Route alignment: on track
- Technical debt: watch
- Validation sufficiency: narrow proof sufficient
- Reviewer recommendation: continue current route
