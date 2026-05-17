Status: Active
Source Idea Path: ideas/open/274_prealloc_regalloc_implementation_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Regalloc Ownership Boundaries

# Current Packet

## Just Finished

Step 1 mapped the current top-level helper clusters in
`src/backend/prealloc/regalloc.cpp` against the active ownership families.

Ownership map summary:

- Value discovery and classification: f128 constant discovery helpers at the
  top of the file, `find_f128_constant_regalloc_value`,
  `classify_register_class`, `resolve_register_class`,
  `resolve_register_group_width`, `register_bank_from_class`, and the initial
  liveness-to-`PreparedRegallocValue`/constraint construction in
  `BirPreAlloc::run_regalloc()`.
- Register class and width decisions: the prepared register-group override
  lookups, target register-bank mapping, and register-name/placement
  materializers used when building constraints and publication records.
- Interval ranking and physical assignment: interval overlap, priority and
  weighted-use scoring, active assignment tracking, candidate span selection,
  eviction, allocation ordering, and the three `assign_from_pool` passes inside
  `run_regalloc()`.
- Stack-slot fallback: existing stack-slot lookup, stack-slot allocation,
  normalized size/alignment helpers, and the final unassigned-value stack
  fallback loop in `run_regalloc()`.
- Value-home publication: `classify_prepared_value_home`,
  `classify_prepared_move_phase`, prepared move-bundle/ABI-binding appenders,
  `append_prepared_call_abi_bindings`, and
  `build_prepared_value_location_function`.
- Call and return ABI move resolution: ABI destination helpers, call argument
  and result storage/destination helpers, return ABI inference, and
  `append_call_arg_move_resolution`, `append_call_result_move_resolution`, and
  `append_return_move_resolution`.
- Phi and out-of-SSA move resolution: parallel-copy lookup/reason helpers,
  `append_phi_move_resolution`, consumer move-resolution helpers, and the
  shared move-resolution appenders.
- Pointer carrier tracking: `PreparedPointerCarrierState`,
  `PreparedPointerCarrierMap`, pointer step/carrier update helpers, carrier
  state resolution, and `build_pointer_carrier_map`; this currently feeds
  value-home publication.
- Runtime-helper mapping: i128/f128 helper opcode, kind, callee, cast, width,
  missing-fact, and append helpers, plus
  `append_i128_runtime_helper_mappings` and
  `append_f128_runtime_helper_mappings`.
- Spill and reload publication: `append_spill_reload_ops`, reusing
  program-point lookup and register-bank/group-width helpers.
- Coordinator: `BirPreAlloc::run_regalloc()` owns phase clearing, per-function
  construction order, stack-layout frame growth, prepared notes, and final
  publication into `prepared_.regalloc` and `prepared_.value_locations`.

## Suggested Next

First extraction packet: extract the pure classification/register metadata
helpers into `src/backend/prealloc/regalloc/classification.cpp` with a narrow
private header, leaving behavior unchanged.

Initial helper set:

- `classify_register_class`
- `resolve_register_class`
- `resolve_register_group_width`
- `register_bank_from_class`
- `materialize_register_names`
- `materialize_register_placements`
- `published_register_group_width`
- `assigned_register_placement`

This is the lowest-risk first slice because it does not own BIR traversal,
move-resolution ordering, stack-frame mutation, or publication vector order.
It proves the private-header pattern before moving larger owners.

## Watchouts

- Preserve allocation decisions and prepared dump output.
- Do not create testcase-shaped shortcuts or expectation downgrades.
- Do not split prepared printer output in this plan.
- Keep implementation progress in this file unless runbook scope changes.
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
- New files under `src/backend/prealloc/regalloc/` should be picked up by the
  recursive prealloc source glob in `src/backend/CMakeLists.txt`; if the build
  does not regenerate automatically, rerun CMake before treating it as a code
  failure.

## Proof

Inventory-only Step 1; no build proof required. Local validation run:
`git diff --check`.

Suggested proof command for the first extraction packet:

```sh
cmake --build build --target c4c_backend backend_prepare_liveness_test backend_prepared_printer_test -j && ctest --test-dir build -R '^(backend_prepare_liveness|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff)$' --output-on-failure
```
