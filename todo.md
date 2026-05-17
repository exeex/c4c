Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract Value-Home Publication

# Current Packet

## Just Finished

Step 4 follow-up extracted behavior-preserving pointer-carrier map construction
out of `src/backend/prealloc/regalloc.cpp` into
`src/backend/prealloc/regalloc/pointer_carriers.cpp` with declarations in
`src/backend/prealloc/regalloc/pointer_carriers.hpp`.

Moved ownership:

- `PreparedPointerCarrierState`
- `PreparedPointerCarrierMap`
- `build_pointer_carrier_map`
- pointer-carrier construction helpers used only by that builder

`src/backend/prealloc/regalloc.cpp` still owns value-location publication order,
move bundle publication, ABI binding publication, runtime-helper mapping,
allocation decisions, and prepared dump sequencing. The extracted builder still
returns the same completed pointer-carrier map consumed by
`classify_prepared_value_home`.

## Suggested Next

Next coherent packet: inspect the remaining value-location publication helpers
for a narrow pure boundary that does not cross move bundle publication, ABI
binding publication, runtime-helper mapping, allocation decisions, or prepared
dump ordering.

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
- `value_homes.cpp` owns classification only. Pointer-carrier discovery still
  lives in `pointer_carriers.cpp`; do not merge pointer-carrier construction
  with value-location publication or runtime-helper mapping in a follow-on
  packet.
- `PreparedPointerCarrierMap` is declared in `pointer_carriers.hpp` so the
  classifier and publication call site can share the carrier result type.
- New `pointer_carriers.cpp` was picked up by the recursive prealloc source glob
  after CMake regenerated during the delegated build.
- `find_regalloc_value`, `assigned_storage_kind`,
  `assigned_storage_matches` now live in `regalloc_detail::storage`; keep that
  file read-only and do not grow it into move publication or allocator
  mutation.
- The overloaded move-resolution appenders were intentionally left in
  `regalloc.cpp`: their duplicate checks and `PreparedMoveResolution`
  construction are coupled to phi, consumer, call, return, immediate,
  f128-constant, ABI, and value-location publication behavior.
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
