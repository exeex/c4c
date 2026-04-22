# Execution State

Status: Active
Source Idea Path: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Prepared Byval-Home Publication/Layout Surface
Plan Review Counter: 1 / 5
# Current Packet

## Just Finished

Idea 76 step `1` is now confirmed. The first bad `fa1` / `fa2` helper homes
are upstream published pointer-value stack homes, not helper-authored outgoing
payload storage and not generic stack-layout frame objects. The concrete
published homes visible in `build/c_testsuite_x86_backend/src/00204.c.s` are
the register-passed helper arguments based at `rsp+6160`, `rsp+6168`,
`rsp+6176`, `rsp+6184`, `rsp+6192`, and `rsp+6200`
([build/c_testsuite_x86_backend/src/00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3451)).
The overlap is already present before `fa1`: `s9` byte 8 lands at `rsp+6176`
and `s10` starts there, `s10` byte 8 lands at `rsp+6184` and `s11` starts
there, then the same reuse happens at `rsp+6192` and `rsp+6200`
([00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3310),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3313),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3337),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3343),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3369),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3376),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3402),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3414)).

The exact upstream producer seam for step `2` is the `regalloc` value-home
publication path in
`src/backend/prealloc/regalloc.cpp`: `PreparedRegallocValue` instances that
`requires_home_slot` are forced into `allocate_stack_slot(...)`
([src/backend/prealloc/regalloc.cpp](/workspaces/c4c/src/backend/prealloc/regalloc.cpp:1894)),
and that allocator uses `normalized_value_size(value)` /
`normalized_value_alignment(value)` rather than aggregate payload size
([regalloc.cpp](/workspaces/c4c/src/backend/prealloc/regalloc.cpp:149),
[regalloc.cpp](/workspaces/c4c/src/backend/prealloc/regalloc.cpp:420)).
For pointer-typed values this normalizes to pointer-width stack slots, then
`classify_prepared_value_home(...)` publishes those offsets as
`PreparedValueHomeKind::StackSlot`
([regalloc.cpp](/workspaces/c4c/src/backend/prealloc/regalloc.cpp:432),
[regalloc.cpp](/workspaces/c4c/src/backend/prealloc/regalloc.cpp:561)),
and `build_prepared_value_location_function(...)` writes them into
`PreparedValueLocationFunction.value_homes`
([regalloc.cpp](/workspaces/c4c/src/backend/prealloc/regalloc.cpp:1088)).

This packet also narrowed what step `2` is not:
- not idea 61's multi-function prepared-module or call-bundle consumption
  surface; the case already reaches runtime, and the helper path uses the
  published value-home/local-slot addresses rather than needing a new module
  traversal or call-bundle consumer
- not idea 68's prepared local-slot handoff consumption; the x86 helper route
  is already consuming published homes, and the bad fact is that those homes
  are wrong upstream
- not idea 71's variadic runtime leaf; the first visible mismatch is still the
  fixed-arity aggregate-string/mixed-aggregate block before `stdarg:`
- not idea 75's helper-contract surface; `backend_x86_handoff_boundary`
  remains green and the helper lane only reads the upstream-published homes

## Suggested Next

Take idea 76 step `2` on one exact seam: inspect and repair
`src/backend/prealloc/regalloc.cpp` where pointer-typed
`PreparedRegallocValue.requires_home_slot` values are stack-assigned and then
published through `PreparedValueHome`. Keep that packet bounded to
`allocate_stack_slot(...)`, `normalized_value_size(...)` /
`normalized_value_alignment(...)`, and `classify_prepared_value_home(...)`.
Do not start in `stack_layout/slot_assignment.cpp`, and do not spend the next
packet on `call_arg_destination_stack_offset_bytes(...)` because x86_64 byval
pointer call args already take the register path
([regalloc.cpp](/workspaces/c4c/src/backend/prealloc/regalloc.cpp:1397)).

## Watchouts

- `call_arg_destination_stack_offset_bytes(...)` is not the right step-`2`
  seam for `fa1` / `fa2`. On x86_64, byval pointer args with a register lane
  are classified as register-consumed
  ([src/backend/prealloc/regalloc.cpp](/workspaces/c4c/src/backend/prealloc/regalloc.cpp:1389)),
  so the helper’s bad addresses do not come from the call-ABI stack-offset
  calculator.
- `stack_layout/slot_assignment.cpp` is also not the first suspected owner for
  this seam. That path lays out `PreparedStackObject`s using each object's
  actual `size_bytes` / `align_bytes`
  ([src/backend/prealloc/stack_layout/slot_assignment.cpp](/workspaces/c4c/src/backend/prealloc/stack_layout/slot_assignment.cpp:91)),
  which is a different surface from the regalloc-published pointer-value homes
  now visible in the emitted `fa1` / `fa2` helper arguments.
- The current proof frontier is unchanged: `backend_x86_handoff_boundary`
  passes, while `00204.c` still first fails in the aggregate string /
  mixed-aggregate block and only later corrupts `Return values:` and `stdarg:`.
- Keep the next packet bounded to the upstream home-publication seam inside
  idea 76. Do not reopen ideas 61, 68, 71, or 75 unless the first bad fact
  moves again after the regalloc publication surface is repaired.

## Proof

Fresh focused proof for idea 76 step `1`:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_MISMATCH]`
  and the first visible mismatch is still the same `fa1` / `fa2`
  aggregate-string / mixed-aggregate seam
- Proof log path: `test_after.log`
