Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Extract ABI Call and Return Move Resolution

# Current Packet

## Just Finished

Step 5 follow-up extracted the call/return move-resolution appender boundary
after the prior call/return ABI helper split.

Moved ownership:

- `append_call_arg_move_resolution`
- `append_call_result_move_resolution`
- `append_return_move_resolution`
- f128 constant call-argument move-record append support
- unassigned return move-record append support

New files:

- `src/backend/prealloc/regalloc/call_moves.cpp`
- `src/backend/prealloc/regalloc/call_moves.hpp`
- `src/backend/prealloc/regalloc/move_records.cpp`
- `src/backend/prealloc/regalloc/move_records.hpp`

`src/backend/prealloc/regalloc.cpp` still owns phi/out-of-SSA move resolution,
consumer move resolution, immediate i32 move-record append support,
spill/reload publication, runtime-helper mapping call order, value-home
publication, and allocation decisions. A narrow shared `move_records` helper
now owns only storage-transfer reason strings and the two duplicate-checking
`append_move_resolution_record` overloads already shared by phi/consumer and
call/return paths.

## Suggested Next

Next coherent packet: move to Step 6 and inspect whether phi/out-of-SSA move
resolution can be extracted cleanly using the existing `move_records` helper.
Keep consumer move resolution separate unless the boundary shows it is truly
part of the same control-flow move owner.

## Watchouts

- Preserve move-resolution ordering, duplicate-check behavior, ABI semantics,
  allocation decisions, prepared dump output, and value-location publication
  order.
- Do not create testcase-shaped shortcuts or expectation downgrades.
- Do not split prepared printer output in this plan.
- Keep implementation progress in this file unless runbook scope changes.
- New `intervals.cpp` was picked up by the recursive prealloc source glob after
  CMake regenerated during the delegated build.
- New `assignment.cpp` was also picked up by the recursive prealloc source glob
  after CMake regenerated during the delegated build.
- `locate_program_point` is intentionally shared by weighted-use scoring and
  spill/reload publication through `regalloc_detail::ProgramPointLocation`.
- The allocation-order comparator remains inline in `run_regalloc()` to
  preserve exact tie order: interval start ascending, priority descending, then
  value id ascending.
- `intervals.cpp` is read-only and must not grow into allocator mutation,
  stack-slot fallback, move resolution, runtime-helper mapping, or publication
  ownership.
- `assignment.cpp` owns read-only candidate/rank helpers only. The templated
  eviction-candidate helper remains header-defined so lambda eviction policies
  in `run_regalloc()` keep their existing call shape.
- `expire_completed_assignments` still mutates the active assignment vectors in
  `regalloc.cpp`; do not move it without a broader allocator mutation packet.
- `value_homes.cpp` owns classification plus ordered value-home publication.
  Pointer-carrier discovery still lives in `pointer_carriers.cpp`; do not merge
  pointer-carrier construction with move-bundle publication, ABI binding
  publication, or runtime-helper mapping in a follow-on packet.
- `PreparedPointerCarrierMap` is declared in `pointer_carriers.hpp` so the
  classifier and publication call site can share the carrier result type.
- New `pointer_carriers.cpp` was picked up by the recursive prealloc source glob
  after CMake regenerated during the delegated build.
- `find_regalloc_value`, `assigned_storage_kind`,
  `assigned_storage_matches` now live in `regalloc_detail::storage`; keep that
  file read-only and do not grow it into move publication or allocator
  mutation.
- f128 constant discovery now lives in `regalloc_detail::values`; its callers
  still cross value appending, call-arg move resolution, and f128 runtime-helper
  mapping, so do not move those caller families as a follow-on without a
  separate packet.
- `stack_slots.cpp` mutates only the explicit `next_slot_id`,
  `next_offset_bytes`, and `frame_alignment_bytes` references passed by
  `run_regalloc()`; do not widen it into allocation decisions or final prepared
  frame-layout publication without a separate allocator-state packet.
- New `stack_slots.cpp` was picked up by the recursive prealloc source glob
  after CMake regenerated during the delegated build.
- New `storage.cpp` was picked up by the recursive prealloc source glob after
  CMake regenerated during the delegated build.
- `build_prepared_value_location_function` was intentionally left in
  `regalloc.cpp`; moving it would require either moving/exposing move-bundle
  publication and call ABI binding publication or introducing callback plumbing
  around those adjacent families.
- `call_moves.cpp` owns only call argument/result and return ABI move
  resolution. Do not grow it into runtime-helper mapping, value-home
  publication, call-plan construction, or allocation policy.
- `move_records.cpp` is intentionally narrow shared append infrastructure.
  Keep immediate materialization, phi/out-of-SSA traversal, consumer traversal,
  runtime-helper mapping, and publication ownership out of it.
- `regalloc.cpp` still owns `append_immediate_i32_move_resolution_record`
  because it is currently used only by phi/out-of-SSA immediate materialization.
- New `call_moves.cpp` and `move_records.cpp` were picked up by the recursive
  prealloc source glob after CMake regenerated during the delegated build.
- `clang-format` is not installed in this environment; the touched declarations
  and call sites were kept manually wrapped.

## Proof

Delegated proof passed and is preserved in `test_after.log`:

```sh
cmake --build build --target c4c_backend backend_prepare_liveness_test backend_prepared_printer_test -j && ctest --test-dir build -R '^(backend_prepare_liveness|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff)$' --output-on-failure
```

Result: build succeeded; 4/4 selected tests passed:
`backend_prepare_liveness`, `backend_prepared_printer`,
`backend_cli_dump_prepared_bir_exposes_contract_sections`, and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`.

Additional validation: `git diff --check` passed.
