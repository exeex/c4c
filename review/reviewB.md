# Idea 652 Step 3 Route Review

Active source idea: `ideas/open/652_prepared_incoming_stack_formal_authority.md`

Chosen base commit: `c46706d45` (`[plan+idea] [plan] Close RV64 stack ABI residual and activate incoming formal authority`)

Why this base: this commit created the active idea 652 source file and reset the active `plan.md`/`todo.md` lifecycle state to the prepared incoming stack formal authority route. Later commits `67af345ab` and `0b075853f` record Step 1 evidence and Step 2 producer fact publication; they do not replace the source idea or create a reviewer checkpoint for the current Step 3 RV64 consumer wiring.

Commit count since base: 2 committed changes plus the current uncommitted Step 3 diff.

Reviewed range: `c46706d45..HEAD` plus the current working-tree diff.

## Findings

### Low: Branch stack-formal consumption has weaker duplicate-formal ambiguity coverage than the LoadLocal path

`src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:940` rejects ambiguous matches by tracking whether more than one formal matches the `LoadLocal` slot name/type before returning the explicit incoming offset. The analogous branch operand helper in `src/backend/mir/riscv/codegen/object_emission.cpp:3642` returns on the first matching formal name/type/size and does not independently prove duplicate-name ambiguity fail-closed behavior.

This is not the prior route drift. The helper calls `plan_prepared_formal_publication()` and consumes `plan.incoming_stack_offset_bytes` at `src/backend/mir/riscv/codegen/object_emission.cpp:3652` and `src/backend/mir/riscv/codegen/object_emission.cpp:3668`; it does not derive the incoming offset from formal order, ABI size/alignment progression, local home offset, source syntax, final assembly, or `stack_frame_bytes`. Still, because the active idea asks for negative coverage around missing, ambiguous, or local-home-only authority, the branch-side positive path would be stronger with a small ambiguous duplicate-formal rejection test or a shared helper that matches the `LoadLocal` ambiguity handling.

Recommended handling: do not block this Step 3 slice. Add ambiguity coverage in Step 4 if the supervisor wants the review/validation boundary to be tighter before lifecycle close.

## Non-Blocking Notes

- The current diff stays on the repaired route from `review/reviewA.md`. I did not find the rejected RV64-side helper that walks formals to compute ABI stack offsets by size/alignment accumulation and then adds `stack_frame_bytes`.
- `src/backend/mir/riscv/codegen/object_emission.cpp:11440` uses `plan_prepared_formal_publication()` during admission and requires `PreparedFormalPublicationAction::IncomingStackToHome`, a matching local home pointer, and `plan.incoming_stack_offset_bytes`; local `PreparedValueHome::offset_bytes` is used only for frame-slot/object coherence checks at `src/backend/mir/riscv/codegen/object_emission.cpp:11462`.
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:915` uses the same explicit publication path for `LoadLocal` formal loads and adds only `incoming_stack_base_bytes + incoming_stack_offset_bytes` at `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:969`.
- Positive RV64 coverage distinguishes explicit incoming authority from local home offsets: `tests/backend/mir/backend_riscv_object_emission_test.cpp:15384` checks branch loading from incoming offset plus frame base and not local home offset; `tests/backend/mir/backend_riscv_object_emission_test.cpp:18307` does the same for `LoadLocal`; `tests/backend/mir/backend_riscv_object_emission_test.cpp:18346` covers an F64 incoming formal with an explicit non-default call-frame base.
- Missing/local-home-only fail-closed coverage remains present at `tests/backend/mir/backend_riscv_object_emission_test.cpp:15376`, `tests/backend/mir/backend_riscv_object_emission_test.cpp:18286`, and `tests/backend/mir/backend_riscv_object_emission_test.cpp:18301`.
- I did not find filename-specific handling for `src/20001017-1.c`, `bug`, `C`, or `fdC`, and I did not find expectation, unsupported-marker, allowlist, timeout, runtime-policy, or pass/fail accounting changes in the current diff.
- The recorded proof in `test_after.log` passed the delegated Step 3 command: 8/8 focused CTests passed and the allowlisted `src/20001017-1.c` RV64 backend object progress check passed.

## Judgments

Idea-alignment judgment: `matches source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `on track`

Technical-debt judgment: `watch`

Validation sufficiency: `narrow proof sufficient`

Reviewer recommendation: `continue current route`

Rationale: Step 3 consumes the explicit producer/prealloc `PreparedFormalPublicationPlan::incoming_stack_offset_bytes` fact and keeps local home offsets as coherence evidence rather than incoming authority. The remaining concern is coverage polish around ambiguous branch-formal matching, not blocking route drift or testcase overfit.
