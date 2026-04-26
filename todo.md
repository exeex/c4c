# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Recover Supported Scalar Rendering Semantics

## Just Finished

Step 3 Recover Supported Scalar Rendering Semantics restored authoritative
prepared value-home rendering for single-block `i32` binary-immediate returns.
The x86 prepared-module consumer now requires the prepared BeforeInstruction
move bundle, source/result value homes, and BeforeReturn move bundle, then
emits the binary operation from either register-backed or stack-backed source
homes into the prepared result register before the prepared return ABI move.

## Suggested Next

Next packet should continue Step 3 on the next scalar blocker from the same
proof: recover semantic prepared-module rendering for the `minimal immediate
return route`.

## Watchouts

Do not turn the immediate return route into a named-fixture dispatcher. The
binary-immediate fix covers the prepared stack-home arithmetic lane
semantically; constant/immediate return rendering still needs to follow the
prepared return/value-location contracts rather than bypassing the handoff.

## Proof

`( cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' ) > test_after.log 2>&1`
rebuilt `tests/backend/backend_x86_handoff_boundary_test` and kept
`backend_x86_prepared_handoff_label_authority` passing. The aggregate proof
advanced past `stack-backed i32 add-immediate route` and the related prepared
stack-home arithmetic lanes, then fails later at `minimal immediate return
route: x86 prepared-module consumer did not emit the canonical asm`.
Canonical proof log: `test_after.log`.
