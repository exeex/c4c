# Step 4 Accumulated Local-Slot Renderer Route Review

## Review Base

- Chosen lifecycle base: `89681abd [plan] rewrite step 4 state after short-circuit review`.
- Rationale: this is the latest lifecycle checkpoint that explicitly reset the active Step 4 route after the prior short-circuit review. The later commits `59729066` and `95045578` are implementation/proof commits and do not reset the plan checkpoint.
- Commits since base: 2.
- Dirty scope reviewed: current worktree diff from `HEAD` in `src/backend/mir/x86/module/module.cpp`, `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`, and `todo.md`, plus `test_after.log`.

## Findings

### High: The dirty slice now mixes Step 4 control-flow recovery with Step 3 scalar local-slot rendering

The short-circuit and local i32 guard paths are not simple fixture-name matches: they require prepared control-flow, addressing, value-home, frame-slot, branch-condition, return-ABI, and in the short-circuit case branch-plan/parallel-copy authority (`src/backend/mir/x86/module/module.cpp:3075`, `src/backend/mir/x86/module/module.cpp:3296`). That part remains directionally aligned with Step 4.

However, the same dirty module diff also adds `append_prepared_i32_local_slot_return_function` for a one-block store/load/return shape and wires it into the generic scalar renderer chain after validation (`src/backend/mir/x86/module/module.cpp:2931`, `src/backend/mir/x86/module/module.cpp:3581`). This is not prepared control-flow recovery; it is standalone scalar local-slot rendering. Combined with `todo.md` now proposing the next blocker as `minimal local-slot i16 increment guard route` (`todo.md:23`), the route has crossed from prepared control-flow identity recovery into broader scalar/local-slot renderer recovery.

This does not prove the local-slot guard/short-circuit code is testcase-overfit by itself, but it means the accumulated dirty slice is no longer a clean Step 4 commit candidate. Split the scalar local-slot return and any i16/increment support into Step 3 or a new focused scalar renderer packet before continuing.

### High: The red proof blocks commit

`test_after.log` shows configure and build passed, and `backend_x86_prepared_handoff_label_authority` passed, but `backend_x86_handoff_boundary` still fails:

`minimal local-slot i16 increment guard route: x86 prepared-module consumer did not emit the canonical asm`

The current `todo.md` correctly says this is not acceptance proof (`todo.md:55`). Because this dirty slice changes renderer dispatch and expected acceptance surfaces, it should not be committed until the supervisor either records the i16 increment route as an explicit unsupported boundary or routes that scalar work through a revised/split plan and obtains fresh green proof.

### Medium: Negative coverage improved, but it is not enough for the widened scalar surface

The new missing-frame-slot negative is useful: it clears `prepared.stack_layout.frame_slots` and requires the x86 consumer to reject the route with a prepared frame-slot authority error (`tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:849`, `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:881`). The adjusted branch-plan helper expectation also compares prepared continuation ids instead of raw drifted block names (`tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:2472`).

The remaining gap is that the new scalar local-slot return route has no corresponding negative coverage in the dirty diff for missing/drifted frame-slot authority, return-move/value-home drift, or mismatched load/store slots. If that helper stays, it needs Step 3-style same-feature positive and negative coverage, not only incidental proof through a larger handoff boundary test.

### Medium: Dispatch ordering still requires a local validation contract

Both local-slot control-flow helpers run before `validate_prepared_control_flow_handoff` (`src/backend/mir/x86/module/module.cpp:3568`, `src/backend/mir/x86/module/module.cpp:3574`). `todo.md` now accurately notes that this path must own equivalent local checks (`todo.md:31`). That is acceptable only while the helpers keep rejecting missing/drifted prepared authority locally. Any next patch that adds raw-label fallback, named fixture selection, or broad "best effort" recovery here would be testcase-overfit and should be rejected.

## Judgments

- Plan-alignment judgment: `drifting`.
- Idea-alignment judgment: `matches source idea`, but the active Step 4 packet is now absorbing Step 3 scalar work.
- Technical-debt judgment: `action needed`.
- Validation sufficiency: `needs broader proof`; current proof is red and therefore not acceptance-ready.
- Testcase-overfit risk: present but not yet decisive for the prepared control-flow helpers. The risk becomes blocking if the next i16 increment work is implemented as another narrow local-slot renderer shortcut instead of a planned scalar renderer rule or explicit unsupported boundary.

## Recommendation

`rewrite plan/todo before more execution`.

Do not commit the accumulated dirty slice as Step 4. First split or narrow the lifecycle state so Step 4 owns only prepared control-flow identity/label/branch-plan recovery and Step 3 or a new scalar packet owns local-slot return, i16 increment, and other scalar-width lowering. After that split, either remove the scalar-local-slot pieces from the Step 4 commit or prove them under the scalar packet with same-feature positive and negative coverage.

