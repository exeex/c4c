# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 rejected an attempted prepared loop-carry countdown renderer in
`src/backend/mir/x86/module/module.cpp`. The attempt consumed prepared loop
labels, branch predicates, loop-carry edge transfers, predecessor execution-site
move bundles, and the prepared entry-prefix path, but it was not committed
because the recognizer was too broad.

The attempt exposed a route-quality blocker: the loop-carry candidate also took
over `minimal non-global equality-against-immediate guard-chain route` and
produced non-canonical asm. Accepting that would mix Step 4 loop-carry rendering
with a nearby guard-chain shape, so the implementation was reverted.

## Suggested Next

Next executor packet should tighten the Step 4 loop-carry recognizer around a
semantic prepared loop-countdown contract before adding more rendering. The
smallest useful packet is to identify why the non-global equality guard-chain
also satisfies the current loop-carry predicate, then add a prepared identity
distinction or an explicit unsupported boundary so the countdown route cannot
claim unrelated guard-chain ownership.

## Watchouts

Do not broaden the loop-carry renderer by raw label spelling or named testcase
shape. The current blocker is specifically a missing prepared identity
distinction between the minimal countdown loop-carry contract and a nearby
non-global equality guard-chain contract. The rejected attempt also touched
existing missing-branch-record rejection behavior while probing the guard-chain
fallout; review that risk before accepting any future slice.

## Proof

Current canonical baseline remains `test_before.log`, red at the active Step 4
loop-countdown blocker. The delegated proof command was:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` failed with `minimal loop-carried join
countdown route: x86 prepared-module consumer rejected the prepared handoff
...`. The rejected attempt's `test_after.log` was discarded with the reverted
implementation.
