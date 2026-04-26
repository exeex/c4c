# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Recover Supported Scalar Rendering Semantics

## Just Finished

Step 3 Recover Supported Scalar Rendering Semantics restored semantic
prepared-module rendering for the minimal same-module global `i32` load return
lane. The x86 prepared-module consumer now accepts a supported single-block
same-module global scalar form with `i32` immediate global stores followed by a
returned `i32` global load, consumes the prepared load result home plus the
authoritative prepared return move, emits the load through the prepared return
ABI register, emits RIP-relative global accesses, and publishes the matching
same-module zero global data definition.

## Suggested Next

Next packet should continue Step 3 on the next scalar blocker from the same
proof: recover semantic prepared-module rendering for the `minimal named
same-module global sub return route`.

## Watchouts

Do not turn the same-module global route into a named-fixture dispatcher. The
new route is gated by supported same-module globals, `i32` immediate stores,
`i32` global loads without pointer-backed addresses, and prepared value-location
authority. The live blocker is still in the same-module global family, but it
adds multiple global loads plus intervening `i32` subtraction rather than a
single loaded value returned directly.

## Proof

`( cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' ) > test_after.log 2>&1`
rebuilt `tests/backend/backend_x86_handoff_boundary_test` and
`tests/backend/backend_x86_prepared_handoff_label_authority_test`, kept
`backend_x86_prepared_handoff_label_authority` passing, and advanced past
`minimal named same-module global return route`. The aggregate proof now fails
later at `minimal named same-module global sub return route: x86 prepared-module
consumer did not emit the canonical asm`.
Canonical proof log: `test_after.log`.
