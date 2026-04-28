# Step 4 Loop Countdown Cleanup Acceptance Review

Objective: review the dirty Step 4 loop-countdown/local-countdown cleanup slice before supervisor acceptance.

## Review Base

Chosen base: `f8280a1c [plan] reset step 4 loop-countdown route`.

Why this base: it is the current `HEAD` and the unambiguous Step 4 lifecycle reset named by the supervisor. The review is therefore over the dirty working tree after that reset, not over an additional committed range.

Commits since base: 0 committed changes. Dirty files reviewed:
- `src/backend/mir/x86/module/module.cpp`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp`
- `todo.md`

## Findings

### No blocking finding: loop-countdown rendering is now gated by prepared identity

The dirty `append_prepared_loop_join_countdown_function` no longer looks like the rejected missing-identity fallback. It requires a prepared control-flow function, a `PreparedJoinTransferKind::LoopCarry`, prepared branch-condition metadata for the loop block, authoritative incoming edge transfers, prepared branch-target lookup for branch carriers, and predecessor-owned prepared parallel-copy bundles before emitting asm.

References:
- `src/backend/mir/x86/module/module.cpp:804`
- `src/backend/mir/x86/module/module.cpp:820`
- `src/backend/mir/x86/module/module.cpp:858`
- `src/backend/mir/x86/module/module.cpp:865`
- `src/backend/mir/x86/module/module.cpp:880`
- `src/backend/mir/x86/module/module.cpp:979`
- `src/backend/mir/x86/module/module.cpp:1014`

This is still a bounded supported shape, but the selection rule is semantic prepared loop-carry/control-flow identity, not function-name dispatch, raw-label spelling, or synthesized `PreparedBranchCondition`.

### No blocking finding: direct same-feature coverage was added

The dirty test file now includes direct loop-carried countdown positives and nearby variants: reversed join-edge order, trivial preheader, transparent preheader chain, prepared init value authority, alternate `eq zero` branch contracts, and raw BIR terminator drift that must still follow prepared branch labels/targets.

References:
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:543`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:701`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:751`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:801`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:851`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1253`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1314`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1377`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1435`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1476`

Negative coverage now exercises drifted branch-condition identity, drifted join-transfer kind, missing parallel-copy publication, drifted entry handoff carrier, and non-authoritative transparent prefixes.

References:
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:919`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:971`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1022`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1071`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1120`

### No blocking finding: stale two-segment local countdown fallback positives were converted into a missing-identity negative

The previous self-baselining local countdown fallback positives are no longer present: there is no `baseline_asm`, no `join_transfers.clear()`, and no hand-injected `PreparedJoinTransferKind::PhiEdge` path in this test file. The remaining two-segment local countdown assertion clears producer-published branch conditions and requires the x86 consumer to reject the route instead of recovering from raw local-slot topology.

References:
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1218`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1234`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1236`
- `tests/backend/backend_x86_handoff_boundary_loop_countdown_test.cpp:1640`

That is the right cleanup direction for the Step 4 missing-identity rule. Local countdown positive support remains parked until real local-slot loop join/edge-transfer/parallel-copy identity is published.

### Low: loop header emission still uses the matched BIR label spelling

The loop block is found through prepared label identity, but the emitted loop label and back edge use `loop_block->label` while body/exit labels are rendered from prepared branch-condition label ids.

References:
- `src/backend/mir/x86/module/module.cpp:741`
- `src/backend/mir/x86/module/module.cpp:854`
- `src/backend/mir/x86/module/module.cpp:895`
- `src/backend/mir/x86/module/module.cpp:1086`
- `src/backend/mir/x86/module/module.cpp:1091`

I do not treat this as a raw-label fallback blocker for this slice because `find_prepared_loop_countdown_block` first matches the BIR block by prepared label spelling, and the added drift tests prove raw terminator labels are ignored. It is still worth normalizing later so all emitted labels are rendered from prepared ids consistently.

### Low: the aggregate delegated proof remains red at a later guard-chain route

`test_after.log` shows configure and build passed, `backend_x86_prepared_handoff_label_authority` passed, and `backend_x86_handoff_boundary` failed at `minimal non-global equality-against-immediate guard-chain route`.

The handoff boundary runner calls `run_backend_x86_handoff_boundary_loop_countdown_tests()` before `run_backend_x86_handoff_boundary_i32_guard_chain_tests()`, so reaching the guard-chain failure proves the loop-countdown subrunner and its cleanup coverage passed in that aggregate execution.

References:
- `tests/backend/backend_x86_handoff_boundary_test.cpp:283`
- `tests/backend/backend_x86_handoff_boundary_test.cpp:287`
- `test_after.log`

This red guard-chain failure blocks claiming Step 4 or the source idea complete. It does not, by itself, block committing this cleanup slice if the supervisor scopes the commit to loop-countdown/local-countdown cleanup and keeps the guard-chain blocker recorded as the next packet.

## Review Question

Is this dirty slice acceptable Step 4 progress/cleanup that can be committed with the known later guard-chain blocker recorded?

Yes, as a bounded cleanup/progress slice. It removes the rejected missing-identity recovery path, adds direct loop-countdown same-feature coverage, converts the two-segment local countdown fallback positives into a missing-identity negative, and records the remaining guard-chain failure in `todo.md`.

Does it still contain overfit, raw-label, missing-identity recovery, or insufficient proof that should be rejected or lifecycle-rewritten?

No blocking overfit or missing-identity recovery is present in the reviewed dirty slice. The renderer remains a bounded supported shape and should not be oversold as general loop lowering, but the current tests make that boundary explicit enough for Step 4 incremental progress. The only proof caveat is that the delegated aggregate command is still red at the later guard-chain route; treat that as the next blocker, not as successful full Step 4 validation.

## Judgments

Plan-alignment judgment: `on track`.

Idea-alignment judgment: `matches source idea`.

Technical-debt judgment: `watch`.

Validation sufficiency: `narrow proof sufficient` for this cleanup slice only; needs broader proof before Step 4/source-idea closure.

Reviewer recommendation: `continue current route`.

Supervisor acceptance note: commit is reasonable if scoped to this dirty slice plus `todo.md`. Do not claim the full delegated handoff subset is green, and keep the guard-chain failure as the next Step 4 packet.
