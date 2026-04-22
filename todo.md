# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Isolate The Exact Lowering / Call-Lane Consumer
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Plan step 2 isolated the exact downstream seam for the `fa4(...)` overlap and
repaired it generically inside the owned x86 consumer. The culprit was the
aligned-byval payload base resolver
`resolve_prepared_named_ptr_published_payload_frame_offset_if_supported(...)`
in [prepared_local_slot_render.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/prepared_local_slot_render.cpp:4732),
which preferred renderer-local `local_layout.offsets` before the authoritative
prepared stack homes. That collapsed the truthful prepared HFA payload home
(`%t38.*` late home rooted at `6272`) into the rendered `[rsp + 352..364]`
lane. The fix now prefers `find_prepared_value_home_frame_offset(...)` first,
so the regenerated `fa4(...)` setup no longer hands `rsi` the collapsed lane:
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3485)
now still passes `s1` via `lea rdi, [rsp + 6264]` but passes the HFA byval
payload via `lea rsi, [rsp + 6272]` instead of the old `lea rsi, [rsp + 352]`.
The stale `[rsp + 352..364]` copy sequence is still emitted immediately above
that call at [00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3454),
but it is no longer the `fa4(...)` consumer handoff. Fresh proof still fails
first in `Arguments:` before `Return values:`, so idea 75 remains active, but
the original `fa4(...)` overlap is now traced to this exact byval-base
resolution seam rather than to upstream publication/layout.

## Suggested Next

Stay on idea 75 and trace the remaining producer of the stale
`[rsp + 352..364]` HFA copy sequence in `arg()` now that `fa4(...)` no longer
consumes it, starting from the float/HFA byval materialization helpers in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` that still emit
those stores before the call.

## Watchouts

- Do not reopen `src/backend/prealloc/**`: the repaired seam confirms the bad
  collapse was inside the downstream x86 byval consumer, not the truthful
  prepared publishers.
- Do not treat the remaining `[rsp + 352..364]` stores as proven harmless just
  because `rsi` now points at `[rsp + 6272]`; fresh proof still fails in
  `Arguments:` and the stale copy lane is still being emitted.
- Do not hand route ownership to idea 77 while the earliest mismatch remains in
  `arg()` before `Return values:`.

## Proof

Fresh proof on 2026-04-22 used:

- `cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$' | tee test_after.log`
- result: still fails, with the first mismatch still inside `Arguments:` in
  `arg()` before `Return values:`
- proof log: `test_after.log`
- supporting inspection:
  `build/c_testsuite_x86_backend/src/00204.c.s:3451-3493`, where the stale HFA
  copy lane remains at `[rsp + 352..364]` but the repaired `fa4(...)` handoff
  now uses `lea rsi, [rsp + 6272]` instead of the collapsed `[rsp + 352]`
