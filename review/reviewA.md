# Idea 644 Step 3 Route Review

Active source idea: `ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md`

Chosen base commit: `49b472411` (`[plan] [plan+todo_only] Activate RV64 object route stack parameter ABI residual`)

Why this base: this is the lifecycle activation commit for the current active source idea and creates the active `plan.md`/`todo.md` pair for idea 644. The later lifecycle commits `9bc2a9919` and `a8e326a4c` record Step 1/2 todo evidence and classification; they do not reset, replace, or reviewer-checkpoint the active source idea. No later `plan.md` source-idea reset was found.

Commit count since base: 7

Reviewed range: `49b472411..8a1ed1685`

## Findings

### High: RV64 consumer reconstructs stack-passed formal incoming offsets from ABI/formal order instead of consuming explicit prepared authority

The active idea's core rule says the object route must not reconstruct parameter placement from source syntax, stack offsets, or final assembly, and must consume explicit prepared ABI/home facts. The closed stack-passed parameter-home idea is even more direct: RV64 must not infer stack argument homes from argument index or ABI folklore.

The new callee-side load paths compute `incoming_offset` by iterating `function.params`, applying local ABI alignment/size formulas, then returning `stack_frame_bytes + incoming_offset`:

- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:1396` computes the incoming offset from parameter ABI order and alignment, and `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:1476` uses `stack_frame_bytes + incoming_offset` as the load offset.
- `src/backend/mir/riscv/codegen/object_emission.cpp:3663` repeats the same incoming-offset computation for value-to-register materialization, and `src/backend/mir/riscv/codegen/object_emission.cpp:3721` returns `stack_frame_bytes + incoming_offset`.

Both helpers do validate that the formal has a coherent prepared stack-slot home and `regalloc.spill_slot` object, which is good fail-closed scaffolding. But those facts describe the callee's local spill-slot home; they do not explicitly publish the incoming caller-stack ABI offset being loaded after the frame is set up. The load offset is still derived inside the RV64 consumer from the formal sequence and ABI formula. This is an unsafe ABI inference under idea 644 and a regression against idea 512's closed contract.

The focused positive test codifies that inference rather than proving a prepared incoming-offset fact. `tests/backend/mir/backend_riscv_object_emission_test.cpp:18330` expects a load from `sp + 64`, which comes from frame size plus recomputed incoming offset. There is negative coverage for malformed local homes, but not for missing/ambiguous explicit incoming-stack-offset authority because no such authority is required by the new path.

Impact: the current Step 3 path can make the present residual advance by teaching RV64 how to rediscover incoming stack layout. That is not named `src/20001017-1.c` overfit, and it is not an expectation downgrade, but it is route drift from the source idea's prepared-facts-only boundary.

Recommended correction: do not build the next callee-side fix on these helpers as written. Require an explicit prepared incoming stack-parameter/home fact, or route the current residual back to producer/prealloc publication if that fact is missing. Keep the existing local-home coherence checks, but make the incoming caller-stack address an explicit prepared authority rather than an RV64-side derivation.

## Non-Blocking Notes

- The RV64 independent GPR/FPR lane work in `src/backend/bir/lir_to_bir/module.cpp`, `src/backend/prealloc/regalloc/call_return_abi.cpp`, and `src/backend/prealloc/regalloc/value_homes.cpp` appears semantically aligned with the source idea. It separates register-lane accounting rather than special-casing the representative row, and `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:3930` adds mixed scalar lane coverage plus a missing-stack-ABI fail-closed check.
- I did not find expectation, unsupported-marker, allowlist, timeout, runtime-comparison, or pass/fail accounting changes in the reviewed diff.
- I did not find named handling for `src/20001017-1.c`; the concern is ABI-authority inference, not testcase-name overfit.
- `todo.md` is dirty in the working tree before this review. I did not inspect that as a reviewer-owned edit or modify lifecycle state.

## Judgments

Idea-alignment judgment: `drifting from source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `drifting`

Technical-debt judgment: `action needed`

Validation sufficiency: `needs broader proof`

Reviewer recommendation: `rewrite plan/todo before more execution`

Rationale: the source idea and plan are still the right contract, but the current implementation path needs to be redirected before the next packet. The rewrite can stay at `todo.md` or `plan.md` level; the source idea itself does not need to change. The next packet should either replace the RV64-side incoming-offset reconstruction with explicit prepared incoming-stack authority, or classify the missing authority as a producer/prealloc gap.
