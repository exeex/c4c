# Review: Idea 214 Route 6 x86 Scalar Source Row

## Scope

- Active source idea: `ideas/open/214_route6_x86_scalar_source_route_debug_row.md`
- Plan: `plan.md`
- Requested diff: `5820dd267..HEAD`
- Chosen base commit: `5820dd267` (`[plan] Activate Route 6 x86 route-debug plan`)
- Base rationale: this is the active plan activation commit for idea 214, and lifecycle history shows the later commits are Step 1-4 execution packets for the same source idea.
- Commits since base: 4

## Verdict

Acceptance-ready.

No blocking findings. The slice matches the source idea's narrow Route 6 diagnostic replacement: it adds one trace row for scalar `i32` call-argument source agreement, uses the existing x86 agreement helper as the gate, leaves `ConsumedPlans` as the compatibility boundary, and does not move wrapper, ABI, call-plan, or emission policy.

## Findings

None blocking.

Observations:

- `src/backend/mir/x86/debug/debug.cpp:59` adds a route-debug renderer helper that iterates BIR calls and arguments, but the actual row gate is `find_consumed_scalar_i32_call_argument_source(...)` at `src/backend/mir/x86/debug/debug.cpp:76`. That helper requires prepared call-plan presence, Route 6 index presence, named `i32` argument shape, Route 6 `ArgumentValue` kind, valid source id, and source-id agreement with the prepared argument at `src/backend/mir/x86/x86.hpp:95`. This is generic over the selected scalar Route 6 row family and is not hardcoded to the test fixture.
- The row emission at `src/backend/mir/x86/debug/debug.cpp:82` reports only route-debug text. It does not alter wrapper selection, call-plan construction, ABI placement, move bundles, or final x86 emission. Existing x86 module emission already consults the same helper at `src/backend/mir/x86/module/module.cpp:3221` and `src/backend/mir/x86/module/module.cpp:3531`; this slice does not change those paths.
- The renderer calls the new helper only inside the trace function loop at `src/backend/mir/x86/debug/debug.cpp:424`; summary output remains on the existing grouped-authority path at `src/backend/mir/x86/debug/debug.cpp:443`. That keeps the selected change to the trace row surface.
- The positive fixture pins the selected row with a Route 6 source record on the `addip0` scalar `i32` argument at `tests/backend/bir/backend_x86_route_debug_test.cpp:1051`, and asserts exact byte-stable text at `tests/backend/bir/backend_x86_route_debug_test.cpp:1687` and `tests/backend/bir/backend_x86_route_debug_test.cpp:1768`.
- Negative route-debug cases cover absent Route 6 facts, invalid source-id data, duplicate/conflict, and Route 6/prepared mismatch through cloned selected-fixture mutations at `tests/backend/bir/backend_x86_route_debug_test.cpp:1155`, `tests/backend/bir/backend_x86_route_debug_test.cpp:1164`, `tests/backend/bir/backend_x86_route_debug_test.cpp:1173`, and `tests/backend/bir/backend_x86_route_debug_test.cpp:1185`, then assert the selected row is absent while the trace/function row remains at `tests/backend/bir/backend_x86_route_debug_test.cpp:1771`.
- No expected strings were weakened or broadened outside the selected Route 6 scalar source row. The new helper `expect_not_contains(...)` at `tests/backend/bir/backend_x86_route_debug_test.cpp:1478` strengthens the negative assertions rather than relaxing existing contracts.

## Validation

- Step 3 recorded a green x86-enabled proof for `backend_x86_route_debug` and `backend_prepared_lookup_helper` after the row and fallback tests landed:

  `cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

- Step 4 recorded a green default-build wrapper/backend proof, and current `test_after.log` confirms:

  `backend_x86_call_boundary_effect_ordering` passed.

  `backend_prepared_lookup_helper` passed.

- The broader `build-x86` aggregate wrapper target `backend_x86_handoff_boundary_test` is unavailable because of unrelated stale compile signatures in `backend_x86_handoff_boundary_joined_branch_test.cpp`. This is an acceptable residual risk for this slice, not a blocker, because the diff does not touch wrapper/emitter files or wrapper expectations and the focused wrapper/backend proof passed.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `acceptable`
- Validation sufficiency: `narrow proof sufficient`
- Reviewer recommendation: `continue current route`

## Residual Risks

- The current canonical `test_after.log` contains the Step 4 wrapper/backend proof, while the Step 3 route-debug proof is recorded in the committed `todo.md` history rather than the current root log. Because no code changed after Step 3, this is not blocking, but the supervisor may choose to rerun the combined route-debug plus wrapper subset before final close if a single acceptance log is desired.
- The unavailable aggregate wrapper target leaves broader wrapper coverage below the ideal acceptance shape. Given the unchanged wrapper/emitter files and green focused wrapper proof, this does not require plan/todo rewrite before acceptance.
