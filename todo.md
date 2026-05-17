Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Pure Value Discovery and Classification Helpers

# Current Packet

## Just Finished

Step 2 extracted the f128 constant value discovery and lookup helper set into
`src/backend/prealloc/regalloc/values.cpp` with narrow declarations in
`src/backend/prealloc/regalloc/values.hpp`.

Moved helpers:

- `is_f128_immediate_constant`
- `f128_constant_prepared_name`
- `append_f128_constant_value`
- `append_f128_constant_values_for_function`
- `find_f128_constant_regalloc_value`

`src/backend/prealloc/regalloc.cpp` now includes the private value-discovery
header and imports only the f128 constant helpers it still calls. Call/return
ABI move resolution, runtime-helper mapping, allocation decisions, publication
order, and prepared dump semantics remain in `regalloc.cpp`.

## Suggested Next

Next coherent packet: extract interval ranking and program-point location
helpers if the supervisor wants to continue Step 2. Keep weighted use scoring
with `locate_program_point` unless the shared helper boundary is made explicit.

## Watchouts

- Preserve allocation decisions and prepared dump output.
- Do not create testcase-shaped shortcuts or expectation downgrades.
- Do not split prepared printer output in this plan.
- Keep implementation progress in this file unless runbook scope changes.
- New files under `src/backend/prealloc/regalloc/` were picked up by the
  recursive prealloc source glob after CMake regenerated during the delegated
  build.
- `ProgramPointLocation` is shared by weighted-use scoring and spill/reload
  publication; keep `locate_program_point` out of a ranking-only extraction
  unless both owners move together or a private shared helper is introduced.
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
