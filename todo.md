Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Isolate Interval Ranking and Physical Assignment

# Current Packet

## Just Finished

Step 3 third extraction moved the stack-slot fallback helper cluster into
`src/backend/prealloc/regalloc/stack_slots.cpp` with declarations in
`src/backend/prealloc/regalloc/stack_slots.hpp`.

Moved ownership:

- `normalized_value_size`
- `normalized_value_alignment`
- `existing_stack_slot_assignment`
- `allocate_stack_slot`

`src/backend/prealloc/regalloc.cpp` now imports stack-slot normalization and
slot-construction helpers from `regalloc_detail`. Allocation decisions,
`requires_home_slot` handling, no-interval stack fallback decisions,
register-eviction fallout, stack layout growth counters, final
`prepared_.stack_layout` frame size/alignment publication, move resolution,
pointer carriers, runtime-helper mapping, value-location publication, and
prepared dump semantics remain in `regalloc.cpp`.

## Suggested Next

Next coherent packet: inspect the remaining shared storage/move-resolution
lookup helpers (`find_regalloc_value`, `assigned_storage_kind`,
`assigned_storage_matches`, and the move-resolution appenders) for a narrow
internal owner, without taking phi, consumer, call, return, runtime-helper, or
value-location publication behavior in the same slice.

## Watchouts

- Preserve allocation decisions and prepared dump output.
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
- `PreparedPointerCarrierMap` and `build_pointer_carrier_map` are pointer
  carrier owners, but `classify_prepared_value_home` consumes their result; do
  not move pointer carriers with value-home publication in the same first
  packet.
- `find_regalloc_value`, `assigned_storage_kind`,
  `assigned_storage_matches`, and the overloaded move-resolution appenders are
  shared across phi, consumer, call, return, runtime-helper, and value-location
  owners; they likely need a small internal lookup/storage header before those
  families are split.
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
