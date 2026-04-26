# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Recover Supported Scalar Rendering Semantics

## Just Finished

Step 3 Recover Supported Scalar Rendering Semantics restored semantic
prepared-module rendering for single-block immediate `i32` returns. The x86
prepared-module consumer now accepts only an instruction-free `return i32
<immediate>` shape for this lane, requires the prepared function return ABI
register destination, and emits the immediate into that narrowed return
register.

## Suggested Next

Next packet should continue Step 3 on the next scalar blocker from the same
proof: recover semantic prepared-module rendering for the `minimal named
immediate-add route`.

## Watchouts

Do not turn the immediate return route into a named-fixture dispatcher. The
immediate return fix uses the prepared function return ABI destination rather
than hard-coded fixture names. The next blocker appears to be the
constant-evaluable named immediate-add lane, not the direct immediate-return
terminator lane.

## Proof

`( cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' ) > test_after.log 2>&1`
rebuilt `tests/backend/backend_x86_handoff_boundary_test` and kept
`backend_x86_prepared_handoff_label_authority` passing. The aggregate proof
advanced past `minimal immediate return route`, then fails later at `minimal
named immediate-add route: x86 prepared-module consumer did not emit the
canonical asm`.
Canonical proof log: `test_after.log`.
