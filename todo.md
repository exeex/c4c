Status: Active
Source Idea Path: ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Add Focused Proof

# Current Packet

## Just Finished

Step 5 completed docs-only focused proof for the accepted AArch64 AAPCS64
call/return/frame target-MIR contract. The stale-claim searches found no
material contradiction where current AArch64 call, return, frame, prologue,
epilogue, memory-return, indirect-call, or variadic consumers still own local
ABI policy instead of the contract.

The remaining search hits are expected: contract prohibitions, historical
reference sections, explicit deferrals, or unrelated later surfaces such as
allocation, memory, F128, atomics, intrinsics, and inline assembly. No code,
test, implementation, plan, source-idea, docs, ledger, or review edits were
required for this proof step.

## Suggested Next

All runbook steps are complete. Supervisor lifecycle review should decide
whether to close, deactivate, or split the active plan; no executor packet is
currently recommended before that review.

## Watchouts

- No `test_after.log` was produced for Step 5 because the delegated proof was
  docs/status-only and required no build or test run.
- Direct `CallRecord` variadic count, outgoing-area totals, return-boundary
  records, explicit special-register role records, full variadic entry
  metadata, and broader target ABI classification/helper-call carriers remain
  documented deferrals for later ideas, not blockers for this plan.
- The existing untracked `review/aarch64_allocation_record_step2_review.md`
  artifact was outside owned files and left untouched.

## Proof

Docs/status-only Step 5. No build or test run was required because Steps 2 and
3 were docs-only and Step 4 required no code changes.

Proof run:

- `rg -n -i "\\b(local|locally|legacy|old)\\b.*\\b(ABI|AAPCS|policy|authority|choose|select|infer|rediscover)\\b|\\b(choose|select|infer|rediscover)\\b.*\\b(argument|result|scratch|stack|frame|save|restore|ABI|AAPCS|policy)\\b|assembly text.*\\b(authority|semantic|ABI|policy)\\b|\\b(bl|blr|ret|stp|ldp|str|ldr|mov)\\b.*\\b(authority|semantic|contract)\\b" src/backend/mir/aarch64 plan.md todo.md`
- `rg -n -i "argument registers|result registers|scratch registers|stack arguments|outgoing call area|save/restore|frame layout|callee-save|caller-saved|x8|x16|x17|x29|x30|variadic" src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md src/backend/mir/aarch64/codegen/*.md todo.md`
- `rg -n -i "must (emit|select|lower).*\\b(bl|blr|ret|stp|ldp|str|ldr|mov)\\b|\\b(bl|blr|ret|stp|ldp|str|ldr|mov)\\b.*must|final (call|return|prologue|epilogue|instruction)|instruction selection" src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md src/backend/mir/aarch64/codegen/*.md todo.md`
- `git diff --check`

Result: stale-claim searches found no material contradiction to the accepted
AAPCS64 contract, and `git diff --check` passed.
