# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reset The Accumulated Scalar Local-Slot Route

## Just Finished

Step 3 extended the prepared scalar local-slot guard renderer through the
`minimal local-slot i16 increment guard route`. The route now recognizes the
prepared i16 load feeding the widened compare operand, consumes prepared WORD
frame-slot accesses including the current size-0 i16 access contract, validates
prepared widened value homes, and renders the i16 load/sext/add/trunc/store
chain before the existing prepared branch-record and target-label handling.

The proof now gets past the local-slot return route, the local-slot i32 guard
route, the local-slot i16 increment guard route, and their prepared ownership
checks. The next red boundary is `minimal local-slot i16/i64 subtract return
route`.

## Suggested Next

Next executor packet should extend the prepared local-slot return renderer to
the `minimal local-slot i16/i64 subtract return route`, consuming the prepared
i16 and i64 frame-slot accesses plus widened arithmetic value homes
semantically while keeping the restored local-slot return, i32 guard, and i16
increment guard routes intact.

## Watchouts

The i16 guard producer currently publishes direct frame-slot memory accesses
with authoritative frame-slot identity but `size=0`; the x86 consumer treats
that as valid for the prepared i16 WORD local-slot route. Narrow temporaries
such as `%t1`, `%t4`, and `%t5` may have `kind=none` homes, while widened
values and the compare condition carry the prepared register homes. Keep raw
local carrier fallback closed when prepared branch metadata is present.

## Proof

Current `test_after.log` is red at the next planned local-slot i16/i64 subtract
return boundary, not at the restored local-slot return, i32 guard, or i16
increment guard route. The command run was:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` failed with `minimal local-slot i16/i64
subtract return route: x86 prepared-module consumer did not emit the canonical
asm`. `test_after.log` contains the full delegated proof output.
