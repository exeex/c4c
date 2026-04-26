# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Recover Supported Scalar Rendering Semantics

## Just Finished

Step 3 Recover Supported Scalar Rendering Semantics restored authoritative
prepared value-home rendering for empty single-block `i32` named-value returns.
The x86 prepared-module consumer now requires the prepared BeforeReturn move
bundle and source value home, then emits the return ABI move from either a
register home or a prepared stack slot home.

## Suggested Next

Next packet should continue Step 3 on the next scalar blocker from the same
proof: recover authoritative prepared stack-home operand rendering for the
`stack-backed i32 add-immediate route`.

## Watchouts

Do not turn the add-immediate route into a named-fixture dispatcher. The
passthrough fix is intentionally limited to the empty single-block named return
case; binary scalar routes still need semantic instruction rendering from
prepared homes plus the prepared return move.

## Proof

`( cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' ) > test_after.log 2>&1`
rebuilt `tests/backend/backend_x86_handoff_boundary_test` and kept
`backend_x86_prepared_handoff_label_authority` passing. The aggregate proof
advanced past `stack-backed i32 parameter passthrough route` and still fails
later at `stack-backed i32 add-immediate route: x86 prepared-module consumer
stopped following authoritative prepared stack homes`. Canonical proof log:
`test_after.log`.
