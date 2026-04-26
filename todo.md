# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Recover Supported Scalar Rendering Semantics

## Just Finished

Step 3 Recover Supported Scalar Rendering Semantics restored semantic
prepared-module rendering for single-block named immediate `i32` binary returns.
The x86 prepared-module consumer now accepts a supported immediate/immediate
`i32` binary whose named result is returned, requires the rematerializable
prepared result home plus the authoritative prepared return move, and emits the
prepared immediate into the narrowed return ABI register.

## Suggested Next

Next packet should continue Step 3 on the next scalar blocker from the same
proof: recover semantic prepared-module rendering for the `minimal named
same-module global return route`.

## Watchouts

Do not turn the rematerialized constant route into a named-fixture dispatcher.
The named immediate-add fix is gated by a supported single-block
immediate/immediate `i32` binary shape and consumes the prepared
`RematerializableImmediate` result home. The next blocker appears to be the
same-module global load return lane, not the constant-expression lane.

## Proof

`( cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' ) > test_after.log 2>&1`
rebuilt `tests/backend/backend_x86_handoff_boundary_test` and
`tests/backend/backend_x86_prepared_handoff_label_authority_test`, kept
`backend_x86_prepared_handoff_label_authority` passing, and advanced past
`minimal named immediate-add route`. The aggregate proof now fails later at
`minimal named same-module global return route: x86 prepared-module consumer did
not emit the canonical asm`.
Canonical proof log: `test_after.log`.
