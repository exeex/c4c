# 613 Step 2 Byval Outgoing Stack Slice Review

Active source idea path: `ideas/open/613_abi_call_result_stack_frame_lowering.md`

Chosen base commit: `722ec6385` (`[plan] Activate 613 ABI call result stack-frame lowering`)

Base rationale: this is the commit that activated the current idea 613 runbook and created the active `plan.md`/`todo.md` pair. The later `832d17948` commit is a Step 1 todo-only diagnostic checkpoint, and `d1e6f28bc` is implementation progress inside the active plan rather than a lifecycle reset.

Commit count since base: 2 committed changes, plus the current uncommitted implementation/todo diff.

## Findings

1. Severity: High - Current slice is synthetic-only progress for the selected row family.

   The source idea permits RV64 consumption only where prepared call/return facts already exist, and explicitly keeps prepared authority production out of scope. The uncommitted code adds a fail-closed consumer for complete prepared outgoing-stack byval facts in `src/backend/mir/riscv/codegen/object_emission.cpp:4433` and `src/backend/mir/riscv/codegen/object_emission.cpp:4524`, but the only positive proof constructs those destination stack facts directly in a synthetic fixture at `tests/backend/mir/backend_riscv_object_emission_test.cpp:2828`. The packet's own todo records that the representative real row, `src/20000808-1.c`, still lacks destination stack offsets and remains at `unsupported_call_abi` (`todo.md:28`).

   This is not a named-file shortcut and the fail-closed checks are directionally correct, but it does not move a current torture row or demonstrate that any existing prepared pipeline emits the facts the consumer requires. Under this idea's contract, that makes the slice premature as Step 2 progress.

2. Severity: Medium - The suggested next step crosses the source idea boundary unless split first.

   `todo.md:50` suggests deciding whether idea 613 can temporarily include prepared call-boundary production of explicit outgoing stack offsets. That is contrary to the source idea's out-of-scope line for prepared authority production and the runbook's core rule to consume explicit prepared facts, not produce missing authority inside the RV64 consumer route. This should be split into a separate source idea or handled by plan-owner before more code work.

3. Severity: Low - The implementation itself is fail-closed and not obviously testcase-shaped.

   The added consumer requires explicit outgoing stack area, matching argument/transport destination offset and size facts, bounds checks, and complete payload coverage (`src/backend/mir/riscv/codegen/object_emission.cpp:4524`). The negative tests cover missing outgoing area, missing transport destination offset, undersized destination size, and destination conflicts (`tests/backend/mir/backend_riscv_object_emission_test.cpp:14451`). Those properties make the code plausible once a real producer path exists, but they do not satisfy this packet's progress requirement by themselves.

## Judgments

Idea-alignment judgment: drifting from source idea.

Runbook-transcription judgment: plan is lossy but usable; `todo.md` now contains a next-step suggestion that should be corrected or split before more execution.

Route-alignment judgment: drifting.

Technical-debt judgment: watch.

Validation sufficiency: needs broader proof in the sense of real-row evidence, not a larger CTest subset. The backend proof is useful for regression safety, but it does not prove source-idea progress for this slice.

Reviewer recommendation: split before commit. Do not accept the current uncommitted slice as idea 613 Step 2 progress unless it is revised to move at least one current ordinary ABI consumer row with already-prepared destination stack facts, or the missing outgoing-stack destination fact production is moved to a separate source idea and this consumer is committed there together with real-row evidence.

Concise verdict: split.
