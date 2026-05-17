Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Pure Value Discovery and Classification Helpers

# Current Packet

## Just Finished

Step 2 extracted the pure classification/register metadata helper set selected
by Step 1 into `src/backend/prealloc/regalloc/classification.cpp` with narrow
declarations in `src/backend/prealloc/regalloc/classification.hpp`.

Moved helpers:

- `classify_register_class`
- `resolve_register_class`
- `resolve_register_group_width`
- `register_bank_from_class`
- `materialize_register_names`
- `materialize_register_placements`
- `published_register_group_width`
- `assigned_register_placement`

`src/backend/prealloc/regalloc.cpp` now includes the private header and imports
only these helpers into its anonymous namespace. BIR traversal, stack fallback
mutation, move resolution, pointer carriers, runtime-helper mapping, and
publication vector ownership remain in `regalloc.cpp`.

## Suggested Next

Next coherent packet: extract the f128 constant discovery helpers only if the
supervisor wants to continue value discovery; otherwise move to a separate
packet for interval ranking helpers. Keep either packet narrow and avoid
combining traversal with allocator mutation.

## Watchouts

- Preserve allocation decisions and prepared dump output.
- Do not create testcase-shaped shortcuts or expectation downgrades.
- Do not split prepared printer output in this plan.
- Keep implementation progress in this file unless runbook scope changes.
- New files under `src/backend/prealloc/regalloc/` were picked up by the
  recursive prealloc source glob after CMake regenerated during the delegated
  build.
- `ProgramPointLocation` is shared by weighted-use scoring and spill/reload
  publication; keep `locate_program_point` out of the first extraction unless
  both owners move together or a private shared helper is introduced.
- `PreparedPointerCarrierMap` and `build_pointer_carrier_map` are pointer
  carrier owners, but `classify_prepared_value_home` consumes their result; do
  not move pointer carriers with value-home publication in the same first
  packet.
- `find_regalloc_value`, `assigned_storage_kind`,
  `assigned_storage_matches`, and the overloaded move-resolution appenders are
  shared across phi, consumer, call, return, runtime-helper, and value-location
  owners; they likely need a small internal lookup/storage header before those
  families are split.
- f128 constant discovery crosses values, call-arg move resolution, and f128
  runtime-helper mapping through `is_f128_immediate_constant` and
  `find_f128_constant_regalloc_value`; extract it only after the private helper
  interface is settled.
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
