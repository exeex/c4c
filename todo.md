# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 rejected a narrower prepared loop-countdown renderer probe after review.
The probe required a real `PreparedJoinTransferKind::LoopCarry` join, a
prepared branch condition bound to that join block, loop-carry edge transfers
for init/backedge traffic, authoritative out-of-SSA parallel-copy move bundles,
and the prepared transparent entry-prefix path before rendering the countdown.

The identity split looked credible: the countdown probe no longer accepted the
nearby `minimal non-global equality-against-immediate guard-chain route`.
However, the required delegated subset still failed at that guard-chain route,
the probe had no direct countdown positive/negative coverage, and it also
touched missing-branch-record rejection behavior outside the countdown path. The
implementation was reverted.

## Suggested Next

Next executor packet should either add a prepared semantic renderer for the
non-global equality-against-immediate guard-chain contract or park countdown
rendering until that guard-chain route has an explicit prepared consumer
boundary. The smallest unblocker is to make the guard-chain route emit canonical
asm from prepared branch metadata instead of falling through to the
contract-first stub.

## Watchouts

Do not broaden the loop-carry renderer by raw label spelling or named testcase
shape. The rejected loop probe distinguished countdown from guard-chain by
requiring an actual `LoopCarry` join transfer plus loop-carry parallel-copy
authority; preserve that direction if the route is retried. Also keep missing
prepared branch metadata rejection behavior out of the countdown slice unless
there is direct coverage for the guard routes it affects.

## Proof

Current canonical baseline remains `test_before.log`, red at the active Step 4
loop-countdown blocker. The delegated proof command was:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` failed with `minimal loop-carried join
countdown route: x86 prepared-module consumer rejected the prepared handoff
...`. The rejected narrowed probe's `test_after.log` was discarded with the
reverted implementation.
