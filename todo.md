Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract Value-Home Publication

# Current Packet

## Just Finished

Step 4 value-home publication extraction moved the pure home classifier into
`src/backend/prealloc/regalloc/value_homes.cpp` with declarations in
`src/backend/prealloc/regalloc/value_homes.hpp`.

Moved ownership:

- `PreparedPointerCarrierState`
- `PreparedPointerCarrierMap`
- `classify_prepared_value_home`

`src/backend/prealloc/regalloc.cpp` still builds the pointer-carrier map,
preserves value-location publication order, appends move bundles, appends call
ABI bindings, and owns runtime-helper mapping. The extracted classifier only
consumes the completed pointer-carrier map and the prepared regalloc value.

## Suggested Next

Next coherent packet: inspect whether pointer-carrier map construction has a
separate clean owner, but keep it out of value-location publication unless the
packet explicitly owns that wider boundary.

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
  lives in `regalloc.cpp`; do not merge pointer-carrier construction with
  value-location publication or runtime-helper mapping in a follow-on packet.
- `PreparedPointerCarrierMap` is declared in `value_homes.hpp` only so the
  classifier and existing builder can share the carrier result type.
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
