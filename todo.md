# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consolidate The Prepared Local-Slot Renderer Route

## Just Finished

Step 3 added a reusable prepared local-slot i32 guard-prefix renderer for
prepared frame-slot stores, delayed local-slot loads, named/immediate binary
operations, prepared value-home registers, and prepared branch compare emission.
The add-chain, sub, and single-successor passthrough lane expectations now
follow the prepared register homes instead of hard-coded `eax`, without adding
an add-chain-specific helper.

## Suggested Next

Next executor packet should decide whether the remaining red
`minimal local-slot byte addressed-guard route` belongs in the same Step 3
prepared local-slot statement renderer or should be parked for plan review as a
separate i8 multi-block guard boundary. Do not fold it into the add-chain work
or add a byte-route-specific helper.

## Watchouts

The i32 prepared prefix renderer intentionally does not take over the earlier
single-block compare-against-immediate i32 guard; that path still falls through
to the existing renderer so the earlier i32 evidence remains unchanged. The
i16 increment guard also reaches its existing prepared widened-home renderer
again after allowing trunc casts through the legacy narrow path.

The current blocker is the later byte addressed-guard lane: it has two prepared
i8 local-slot compare blocks and still falls to the unsupported stub. Extending
this packet further would broaden beyond the add-chain scalar boundary into
multi-block i8 guard rendering.

## Proof

Current `test_after.log` is red at `minimal local-slot byte addressed-guard
route`, after the add-chain, sub, passthrough, local-slot return, i32 guard,
i16 increment guard, and parked i16/i64 subtract-return evidence. The command
run was:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` failed with `minimal local-slot byte
addressed-guard route: x86 prepared-module consumer did not emit the canonical
asm`. `test_after.log` contains the full delegated proof output.
