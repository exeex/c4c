# Step 4 Local-Slot Short-Circuit Renderer Review

## Review Base

- Chosen lifecycle base: `89681abd [plan] rewrite step 4 state after short-circuit review`.
- Rationale: this is the latest `plan.md` checkpoint that explicitly rewrote the active Step 4 state after the prior short-circuit route review. The later commits `59729066` and `95045578` are implementation/proof commits, and the current dirty slice was produced after `95045578`; they did not reset the plan checkpoint.
- Commits since base: 2.
- Dirty scope reviewed: `src/backend/mir/x86/module/module.cpp`, `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`, and `todo.md`, plus `test_after.log`.

## Findings

### Medium: Dirty slice is not the earlier fixture-shaped route, but it is not commit-ready while the delegated proof is red

The new `append_prepared_local_slot_short_circuit_or_guard_function` path in `src/backend/mir/x86/module/module.cpp:3005` is materially different from the earlier rejected short-circuit route. It does not identify the known fixture by function name, printed LLVM text, instruction count alone, or raw local-slot labels. It requires prepared control-flow/value-location/addressing records (`src/backend/mir/x86/module/module.cpp:3018`), authoritative branch-owned parallel-copy bundles (`src/backend/mir/x86/module/module.cpp:3065`), a prepared short-circuit branch plan (`src/backend/mir/x86/module/module.cpp:3097`), prepared frame-slot memory accesses (`src/backend/mir/x86/module/module.cpp:2830`), prepared value homes for compare carriers (`src/backend/mir/x86/module/module.cpp:2885`), and prepared return ABI authority (`src/backend/mir/x86/module/module.cpp:2953`).

That makes the slice a legitimate bounded semantic renderer advance for the local-slot short-circuit or-guard family, not a repeat of the rejected testcase/fixture-shaped renderer. The added missing-frame-slot negative in `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:849` also proves that the consumer rejects loss of prepared frame-slot authority instead of falling back to raw local-slot names.

However, `test_after.log` shows the delegated command still fails `backend_x86_handoff_boundary` at `minimal local-slot short-circuit branch-plan prepared label ownership: shared helper stopped publishing the authoritative short-circuit branch-plan labels`. This is a real Step 4 prepared-producer/helper blocker, so the slice should not be committed yet as completed progress. Continuing into that helper blocker before committing is appropriate.

### Medium: `todo.md` overstates the renderer as already behind the broad validator

`todo.md:25` says the local-slot short-circuit renderer "still runs behind the broad prepared control-flow validator", but the code calls `append_prepared_local_slot_short_circuit_or_guard_function` before `validate_prepared_control_flow_handoff` in `src/backend/mir/x86/module/module.cpp:3277`. The new renderer performs many of the right semantic checks itself, so this is not automatically a route failure, but the lifecycle note is inaccurate.

Before accepting the slice, either route this renderer through the same shared validation contract where feasible, or update `todo.md` to state the narrower truth: this path bypasses the broad validator entry point and must therefore own equivalent local checks for branch-plan, continuation-label, parallel-copy, value-home, frame-slot, and return-ABI authority.

### Low: Negative coverage is directionally correct but still incomplete for acceptance

The new missing-frame-slot negative at `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:881` is useful and should stay near the positive local-slot route. Existing nearby tests cover drifted continuation labels and multiple prepared-control-flow ownership cases, and the next failing helper test at `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:3088` is exactly in the prepared-label publication family.

For final Step 4 acceptance, the local-slot renderer still needs the branch-plan helper blocker fixed semantically and the currently unreachable downstream same-feature negatives to run. The current red proof is enough to justify continuing the packet, but not enough to claim the route is complete.

## Judgments

- Plan-alignment judgment: `drifting` only because the dirty slice is not proof-green and `todo.md` misstates the validator ordering; the implementation direction itself matches Step 4.
- Idea-alignment judgment: `matches source idea`.
- Technical-debt judgment: `watch`.
- Validation sufficiency: `needs broader proof` before acceptance; current narrow proof is a useful blocker discovery, not acceptance proof.

## Recommendation

`narrow next packet`.

Do not reject this as the same testcase-overfit route from the earlier review. The renderer is bounded and prepared-metadata driven enough to continue.

Do not commit the dirty slice yet. First repair the exposed branch-plan helper/publication blocker semantically, rerun the same delegated proof, and make the `todo.md` validator-ordering statement accurate. If the helper fix requires adding local fallback labels, raw-label topology recovery, or expectation downgrades, stop and send the route back for plan/todo rewrite instead.
