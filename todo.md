# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consolidate The Prepared Local-Slot Renderer Route

## Just Finished

Step 3 fixed the i16 widened local-slot arithmetic authority bug without adding
another subtract-return helper. The i16 guard renderer now requires prepared
register homes for the widened sext and binary results, emits the prepared home
registers it consumes, and the i16 guard test mutates the binary result home to
prove the emitted register follows prepared value-location authority instead of
hard-coded `eax`.

The `minimal local-slot i16/i64 subtract return route` is now explicitly parked
as unsupported: the test expects the contract-first unsupported stub and rejects
accidental scalar subtract rendering. This keeps the red boundary from being
claimed through another boundary-specific local-slot helper.

## Suggested Next

Next executor packet should address the still-red
`minimal local-slot add-chain guard route` only through the reusable prepared
local-slot expression/statement renderer requested by Step 3, or keep it parked
for plan review. Do not add an add-chain-specific helper or broaden into Step 4
control-flow authority while this scalar consolidation remains unresolved.

## Watchouts

The i16 guard producer currently publishes direct frame-slot memory accesses
with authoritative frame-slot identity but `size=0`; the x86 consumer still
treats that as valid for the prepared i16 WORD local-slot route. Narrow
temporaries such as `%t1`, `%t4`, and `%t5` may have `kind=none` homes, while
widened values and the compare condition carry prepared register homes.

The exact delegated proof is now red at `minimal local-slot add-chain guard
route`. That fixture lives outside this packet's owned test files and the
runbook names add-chain guard as another boundary that must not be solved by
an exact-sequence scalar helper. Keep raw local carrier fallback closed when
prepared branch metadata is present.

## Proof

Current `test_after.log` is red at the local-slot add-chain guard boundary,
not at the restored local-slot return, i32 guard, i16 increment guard, or the
now-parked i16/i64 subtract-return boundary. The command run was:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` failed with `minimal local-slot
add-chain guard route: x86 prepared-module consumer did not emit the canonical
asm`. `test_after.log` contains the full delegated proof output.
