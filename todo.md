# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reset The Accumulated Scalar Local-Slot Route

## Just Finished

Step 3 restored the first scalar local-slot unsupported boundary by adding a
prepared local-slot return renderer in `src/backend/mir/x86/module/module.cpp`.
The route consumes prepared frame-slot memory accesses by block label and
instruction index, prepared value homes for local loads, and the prepared return
move before emitting the minimal local-slot return asm. It does not reintroduce
the retired exact-sequence scalar helpers.

The proof now gets past `minimal local-slot return route` and its prepared
frame-slot access ownership check. The next red boundary is
`minimal local-slot compare-against-immediate guard route`.

## Suggested Next

Next executor packet should extend the same prepared local-slot
expression/statement renderer to the minimal local-slot compare-against-
immediate guard route, consuming authoritative prepared branch records and
prepared frame-slot accesses without reopening raw local carrier fallback.

## Watchouts

The local-slot return renderer intentionally claims only single-block i32 return
functions with prepared local store/load memory accesses. It validates the
prepared load home and return move, then coalesces a returned load directly into
the prepared ABI return register to preserve the canonical asm.

## Proof

Current `test_after.log` is red at the next planned local-slot guard boundary,
not at the restored return route. The command run was:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` failed with
`minimal local-slot compare-against-immediate guard route: x86 prepared-module
consumer did not emit the canonical asm`. `test_after.log` contains the full
delegated proof output.
