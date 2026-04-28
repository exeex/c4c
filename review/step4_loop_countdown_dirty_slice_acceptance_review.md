# Step 4 Loop Countdown Dirty Slice Acceptance Review

Objective: review the current dirty Step 4 loop-countdown slice before supervisor acceptance.

## Review Base

Chosen base: `4dc6bf83 [plan] [plan+todo_only] consolidate step 3 local-slot renderer route`.

Why this base: it is the latest lifecycle checkpoint touching `plan.md`/`todo.md` for the active idea before the current Step 4 execution chain. Later commits are implementation or `todo_only` execution churn under the same idea 121 runbook; the current dirty slice should be judged as drift from this checkpoint.

Commits since base: 7.

## Findings

### High: synthesized local-slot guard conditions are missing-identity recovery

The dirty slice changes `append_prepared_local_slot_immediate_guard_function` so a local-slot conditional branch with no `PreparedBranchCondition` can synthesize one from the BIR compare instruction whenever there are at least two local-slot conditional branches and one prepared true/false target block branches onward.

References:
- `src/backend/mir/x86/module/module.cpp:4278`
- `src/backend/mir/x86/module/module.cpp:4292`
- `src/backend/mir/x86/module/module.cpp:4294`
- `src/backend/mir/x86/module/module.cpp:4296`
- `src/backend/mir/x86/module/module.cpp:4300`
- `src/backend/mir/x86/module/module.cpp:4322`
- `src/backend/mir/x86/module/module.cpp:4364`

This is not just rendering from published prepared identity. It fabricates the missing branch-condition record at the x86 consumer from BIR shape and prepared block targets. Step 4 says that if the x86 consumer reaches missing or drifted prepared identity, the producer records must be repaired or the boundary documented unsupported before adding an x86-side acceptance path. It also says missing and drifted label ids should be rejected or surfaced rather than recovered through consumer-side exceptions.

References:
- `plan.md:226`
- `plan.md:227`
- `plan.md:238`
- `plan.md:239`
- `plan.md:247`
- `plan.md:263`
- `plan.md:264`

This part of the dirty slice should be rejected or sent back for lifecycle review. If the two-segment local countdown needs branch-condition identity, the semantic route is producer publication or an explicit unsupported boundary, not synthesizing `PreparedBranchCondition` in the renderer.

### High: delegated proof is red and blocks acceptance

The delegated command configured and built, and `backend_x86_prepared_handoff_label_authority` passed, but `backend_x86_handoff_boundary` still failed:

```text
minimal non-global equality-against-immediate guard-chain route: x86 prepared-module consumer did not emit the canonical asm
```

References:
- `test_after.log`
- `todo.md:37`
- `todo.md:43`
- `todo.md:44`

This can be treated only as diagnostic partial movement: the loop-countdown route appears to advance far enough for the aggregate proof to reach the next guard-chain blocker. It is not an acceptance proof. The exact delegated proof remains red, so the dirty code slice is not acceptance-ready.

### Medium: loop-countdown rendering is semantically gated, but still needs split and proof

`append_prepared_loop_join_countdown_function` is materially better than raw-label or named-fixture matching. It requires an x86-64 no-parameter i32 function, a real prepared control-flow function, a `LoopCarry` join transfer, exactly two incoming edge transfers, a prepared branch condition comparing the loop carrier to zero, prepared branch target lookup, and predecessor-owned parallel-copy bundles before emitting asm.

References:
- `src/backend/mir/x86/module/module.cpp:804`
- `src/backend/mir/x86/module/module.cpp:809`
- `src/backend/mir/x86/module/module.cpp:820`
- `src/backend/mir/x86/module/module.cpp:831`
- `src/backend/mir/x86/module/module.cpp:858`
- `src/backend/mir/x86/module/module.cpp:865`
- `src/backend/mir/x86/module/module.cpp:890`
- `src/backend/mir/x86/module/module.cpp:979`

That gives the loop-countdown branch a plausible semantic-control-flow basis. However, the renderer is still a narrow supported shape: i32-only, offset-zero local carrier, compare against zero, subtract immediate one, exactly two join edges, hard-coded `eax` emission, and no new same-feature positive or negative coverage in this dirty diff. It should not be accepted as a completed Step 4 slice while bundled with the synthesized missing-identity fallback and a red delegated proof.

### Medium: loop header emission leaks raw BIR label text

The renderer computes prepared label text for the body and exit labels from `PreparedBranchCondition` target ids, but it emits the loop header label using `loop_block->label`.

References:
- `src/backend/mir/x86/module/module.cpp:895`
- `src/backend/mir/x86/module/module.cpp:897`
- `src/backend/mir/x86/module/module.cpp:904`
- `src/backend/mir/x86/module/module.cpp:1086`
- `src/backend/mir/x86/module/module.cpp:1091`

Because the loop block was found by prepared label id, this is not broad raw-string matching. Still, Step 4's completion check asks for prepared label-id consumption directly and rejection of invalid identity. Emitting the prepared join label text consistently would avoid turning this into a raw-label spelling dependency.

### Medium: no direct same-feature tests were added

The dirty diff touches only `src/backend/mir/x86/module/module.cpp` and `todo.md`; it adds no direct loop-countdown positive test, no nearby countdown variant, and no negative coverage for missing or drifted loop branch conditions, join transfers, edge transfers, or parallel-copy bundles.

References:
- `plan.md:250`
- `plan.md:252`
- `ideas/open/121_x86_prepared_module_renderer_recovery.md:48`
- `ideas/open/121_x86_prepared_module_renderer_recovery.md:50`
- `ideas/open/121_x86_prepared_module_renderer_recovery.md:52`

Existing aggregate movement is useful, but the source idea and Step 4 plan require nearby same-feature and negative coverage before treating the route as semantic renderer progress.

### Medium: `todo.md` overstates completion

`todo.md` says Step 4 repaired the loop-countdown blocker and that the path now keeps the two-segment countdown proof moving by synthesized local countdown fallback. The proof section records the red delegated result accurately, but the "Just Finished" language treats an unaccepted dirty slice as repaired.

References:
- `todo.md:9`
- `todo.md:11`
- `todo.md:15`
- `todo.md:32`
- `todo.md:43`

The todo state should describe this as a blocked or unaccepted attempt unless the supervisor deliberately accepts a narrower lifecycle route. The synthesized fallback should not be recorded as progress without plan-owner review because it conflicts with the Step 4 missing-identity rule.

## Review Question

Is this dirty slice acceptable semantic prepared control-flow progress?

No, not as a single accept-and-commit slice. The loop-countdown renderer has real semantic gates and does not look like pure testcase-name dispatch, but the dirty slice also introduces consumer-side missing-identity recovery by synthesizing branch conditions, has no direct same-feature positive/negative coverage, leaks raw BIR label text for the loop header, and leaves the delegated proof red.

Does it rely on testcase-shaped rendering/raw-label or missing-identity recovery?

It does not primarily rely on named testcase dispatch. It has narrow supported-shape rendering and one raw-label spelling leak for the loop header. The blocking issue is missing-identity recovery: synthesizing `PreparedBranchCondition` in the x86 consumer when the prepared producer did not publish one.

Can the red delegated proof be treated as partial progress?

Only as diagnostic partial movement. It shows the failure advanced from loop-countdown to guard-chain, but it is a blocking acceptance failure for this code slice because the supervisor-selected proof command still fails.

## Judgments

Plan-alignment judgment: `drifting`.

Idea-alignment judgment: `matches source idea` in renderer-recovery intent, but the current dirty slice violates the missing-identity and proof requirements.

Technical-debt judgment: `action needed`.

Validation sufficiency: `needs broader proof`; immediately, it first needs the delegated narrow proof green plus direct same-feature positive/negative coverage.

Reviewer recommendation: `rewrite plan/todo before more execution`.

Minimum acceptable next route: split or remove the synthesized branch-condition fallback; repair producer publication for the two-segment countdown identity or record that missing-identity boundary as unsupported; then prove the loop-countdown renderer with direct positive and missing/drifted identity negatives before returning to the guard-chain blocker.
