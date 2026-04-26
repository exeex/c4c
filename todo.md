# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Recover Supported Scalar Rendering Semantics

## Just Finished

Step 3 Recover Supported Scalar Rendering Semantics restored semantic
prepared-module rendering for the minimal same-module global `i32` sub return
lane. The x86 prepared-module consumer now accepts a supported single-block
same-module global scalar expression form with `i32` immediate global stores,
same-module `i32` global loads, and `i32` subtraction over immediate or tracked
named operands; validates prepared register homes, binary instruction move
bundles, and the authoritative prepared return move; emits RIP-relative global
accesses; and keeps same-module data definition emission on the existing data
path.

## Suggested Next

Next packet should move to the next blocker exposed by the same proof:
recover prepared-module rendering for the `scalar-control-flow
compare-against-zero branch lane`.

## Watchouts

Do not turn the same-module global route into a named-fixture dispatcher. The
new route is gated by supported same-module globals, `i32` immediate stores,
`i32` global loads without pointer-backed addresses, `i32` subtraction, and
prepared value-location authority. The current route renders into a local
buffer and only publishes assembly after the whole supported shape is accepted;
keep that all-or-nothing behavior for later candidate renderers so unsupported
control-flow forms can still fall through cleanly.

## Proof

`( cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' ) > test_after.log 2>&1`
rebuilt `tests/backend/backend_x86_handoff_boundary_test` and
`tests/backend/backend_x86_prepared_handoff_label_authority_test`, kept
`backend_x86_prepared_handoff_label_authority` passing, and advanced past
`minimal named same-module global sub return route`. The aggregate proof now
fails later at `scalar-control-flow compare-against-zero branch lane: x86
prepared-module consumer did not emit the canonical asm`.
Canonical proof log: `test_after.log`.
