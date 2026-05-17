Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Isolate Interval Ranking and Physical Assignment

# Current Packet

## Just Finished

Step 3 second extraction moved the read-only physical-assignment candidate
helpers into `src/backend/prealloc/regalloc/assignment.cpp` with a narrow
private header in `src/backend/prealloc/regalloc/assignment.hpp`.

Moved ownership:

- `ActiveRegisterAssignment`
- `assignments_overlap`
- `choose_register_span`
- `has_lower_allocation_rank`
- `choose_eviction_candidate`

`src/backend/prealloc/regalloc.cpp` now includes the private assignment helper
header and imports the active-assignment record plus read-only candidate/span
selection helpers. Active-list expiry, physical assignment mutation, eviction
mutation, stack-slot fallback, move resolution, pointer carriers,
runtime-helper mapping, value-location publication, and prepared dump semantics
remain in `regalloc.cpp`.

## Suggested Next

Next coherent packet: inspect whether the remaining stack-slot fallback helpers
can move behind a focused allocator/stack-fallback owner without taking
prepared value-home publication, move resolution, pointer carriers, or runtime
helper mapping with them.

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
- `allocate_stack_slot` mutates `next_slot_id`, `next_offset_bytes`, and
  `frame_alignment_bytes` from `run_regalloc()` and reads stack-layout
  placement; keep it with allocator/stack fallback rather than the first
  classification slice.

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
