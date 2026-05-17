Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Extract Phi and Out-of-SSA Move Resolution

# Current Packet

## Just Finished

Step 6 follow-up extracted the remaining consumer move-resolution traversal out
of `src/backend/prealloc/regalloc.cpp` without changing behavior.

Moved ownership:

- `append_consumer_move_resolution`
- consumer block-label resolution for select-materialized join suppression
- consumer Binary/Select/Cast operand traversal and move-record emission

New files:

- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`

The new consumer owner depends only on BIR traversal, prepared control-flow
metadata for select-materialized join filtering, regalloc value lookup, and the
existing shared `move_records` append helpers. Runtime-helper mapping,
spill/reload publication, value-location publication, allocation decisions, and
prepared move-bundle publication remain in `src/backend/prealloc/regalloc.cpp`.
An unused local `find_instruction_index_for_named_result` helper was removed
while moving the adjacent consumer block.

## Suggested Next

Next coherent packet: continue Step 6 by inspecting whether spill/reload
publication has a clean owner boundary separate from allocation decisions,
runtime-helper mapping, value-location publication, and prepared move-bundle
publication.

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
- New `call_moves.cpp` and `move_records.cpp` were picked up by the recursive
  prealloc source glob after CMake regenerated during the delegated build.
- `phi_moves.cpp` owns only phi/out-of-SSA parallel-copy move-record emission,
  including i32 immediate materialization for those parallel-copy moves. Do not
  grow it into consumer move reconstruction, runtime-helper mapping, prepared
  move-bundle publication, or allocation policy.
- New `phi_moves.cpp` was picked up by the recursive prealloc source glob after
  CMake regenerated during the delegated build.
- `consumer_moves.cpp` owns only consumer-shaped move-record emission for
  Binary/Select/Cast results. Do not grow it into out-of-SSA parallel-copy
  traversal, runtime-helper mapping, spill/reload publication, value-home
  publication, or prepared move-bundle publication.
- Select-materialized joins remain filtered through prepared control-flow
  authority before consumer move emission; preserve this ordering and the
  existing duplicate-check behavior in `move_records.cpp`.
- New `consumer_moves.cpp` was picked up by the recursive prealloc source glob
  after CMake regenerated during the delegated build.
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
