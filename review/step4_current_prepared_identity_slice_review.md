# Step 4 Current Prepared Identity Slice Review

## Review Base

- Chosen lifecycle base: `c77da3fe [plan] route step 4 through prepared producer identity repair`.
- Rationale: `5dbb6a07` activated the idea 121 plan, but `c77da3fe` is the later lifecycle checkpoint that rewrote Step 4 around the prior review's producer-identity repair route. The current review question is specifically the uncommitted producer/value-location identity slice after that checkpoint.
- Commits since base: 0.
- Dirty scope reviewed: `src/backend/prealloc/prealloc.cpp`, `src/backend/prealloc/prealloc.hpp`, `src/backend/prealloc/regalloc.cpp`, `src/backend/prealloc/prepared_printer.cpp`, `src/backend/mir/x86/module/module.cpp`, `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`, and `todo.md`.
- Prior report consulted: `review/step4_current_x86_control_flow_route_review.md`.

## Findings

### Medium: Return ABI inference is producer-side only for some x86 return routes

`src/backend/prealloc/regalloc.cpp:1922` adds scalar return ABI inference when `bir::Function::return_abi` is missing, and `append_return_move_resolution` uses it at `src/backend/prealloc/regalloc.cpp:2127`. That is a semantic producer repair for the immediate compare-join blocker, not a fixture-shaped shortcut.

However, the x86 consumer is not uniformly moved to the prepared return-move authority. `append_prepared_i32_leaf_return` still rejects named leaf returns when `function.return_abi` is absent at `src/backend/mir/x86/module/module.cpp:1863`, even though the producer may now publish a valid prepared return move for the same semantic condition. The current selected compare-join arm path consumes the return move directly, so this is not blocking the dirty slice, but nearby branch-return coverage could still fail on stale ABI presence rather than prepared value-location authority.

Before treating the return ABI inference as a broader renderer capability, add or run a nearby same-feature leaf/control-flow case without explicit `return_abi`, or narrow the next packet to the global compare-join blocker while keeping this gap recorded in `todo.md`.

### Medium: Fixture metadata refresh is large and partly duplicated, but still repairing post-prepare mutations rather than hiding consumer drift

The joined-branch test now refreshes prepared labels, parallel-copy predecessor labels, value homes, bridge blocks, and return bundles after deliberately mutating an already prepared fixture. The key edits at `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp:2184`, `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp:2217`, and `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp:2677` align the mutated BIR topology with prepared control-flow records, which directly addresses the prior report's missing/drifted identity concerns.

The risk is maintainability rather than overfit: the return-bundle creation logic appears both in the new helper at `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp:1936` and inline later at `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp:2337` and `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp:2597`. That is fixture maintenance debt and could mask future test mistakes if copied again. It does not currently weaken supported expectations or add a consumer-side escape, but the next packet should avoid adding more inline fixture metadata writers; reuse the helper or move common mutation repair into focused test support.

### Low: Immediate-source authority has the right negative check but only one selected case proves it

`PreparedMoveResolution::source_immediate_i32` in `src/backend/prealloc/prealloc.hpp:761`, the producer emission at `src/backend/prealloc/regalloc.cpp:1683`, and the consumer drift check at `src/backend/mir/x86/module/module.cpp:1939` form a real prepared value-location authority contract. The new negative test at `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp:4908` mutates that authority and expects rejection, which is the right shape for Step 4.

The proof is still narrow: it covers the immediate selected-value compare-join route that exposed the blocker. That is acceptable for this packet, but not enough to close Step 4 or claim global compare-join readiness.

## Overfit Assessment

The current dirty slice is still primarily semantic prepared identity/value-location authority repair. The implementation changes publish BIR label identity from prepared control-flow records, preserve immediate source authority across out-of-SSA parallel-copy moves, infer scalar return ABI for prepared return moves, and strengthen x86 validation for incomplete compare authority and drifted immediate sources.

I did not find a new expected-assembly fallback, function-name dispatcher, raw-label recovery path, or compare-join-specific validator exception in the dirty diff. The prior rejected consumer-side escape route should remain rejected, but this slice does not reintroduce it.

## Validation

`test_after.log` records the delegated proof command. `backend_x86_prepared_handoff_label_authority` passed, and `backend_x86_handoff_boundary` advanced past the previous `branch_join_immediate_then_xor` missing return move bundle blocker before failing later at `branch_join_global_then_xor` with an unsupported global compare-join return context.

That is useful blocker advancement, not acceptance proof. The slice still needs fresh supervisor-side validation before commit: at minimum, rerun the same x86 subset from a clean build state or with an explicit build target proof, and preserve the known later global blocker in `test_after.log`. Broader backend validation can wait until the global/value-location route lands or the supervisor treats Step 4 as a milestone.

## Judgments

- Plan alignment: `on track`.
- Idea alignment: `matches source idea`.
- Technical debt: `watch`.
- Validation sufficiency: `needs broader proof`.

## Recommendation

`narrow next packet`.

Supervisor should not reject this dirty slice as testcase-overfit. After additional validation, it is reasonable to commit this producer/value-location identity repair as a coherent Step 4 slice with the updated `todo.md`.

The next packet should stay narrow on the exposed same-module global compare-join value-location blocker. Do not broaden into general same-module global compare-join return support unless the packet proves the prepared global/value-location contract semantically and adds same-feature positive and negative coverage.
