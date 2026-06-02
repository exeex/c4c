Status: Active
Source Idea Path: ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the Existing Frame-Slot Address Boundary

# Current Packet

## Just Finished

Step 1 mapped the existing AArch64 frame-slot address helper boundary without
changing implementation code.

Chosen minimal owner shape: add `src/backend/mir/aarch64/codegen/frame_slot_address.hpp`
and `src/backend/mir/aarch64/codegen/frame_slot_address.cpp`, then list the new
`.cpp` in `src/backend/CMakeLists.txt` next to the other AArch64 codegen units.
The header should publish only AArch64 frame-slot address helpers and required
prepared-stack lookup accessors; it should not become a generic memory, storage
plan, dynamic-stack, `va_list`, dispatch-publication, or register-spelling
authority.

Current helper categories in the target boundary:

- Address text/base selection: `fixed_slots_use_frame_pointer`,
  `frame_slot_address(offset, base_register)`, and
  `frame_slot_address(function, offset)` currently live in `memory.cpp` and are
  declared in `memory.hpp`.
- Prepared frame-slot lookup: local `find_frame_slot` and `find_stack_object`
  wrappers currently scan `PreparedStackLayout` in `memory.cpp`.
- Prepared load offset/address helpers: `prepared_frame_slot_load_address` and
  `prepared_local_load_offset` combine prepared addressing facts with frame-slot
  offsets and frame-pointer-vs-SP base selection.
- Prepared operand construction/materialization:
  `make_prepared_frame_slot_memory_operand` builds record-only `MemoryOperand`
  values for prepared frame slots, and
  `materialize_frame_slot_memory_address_lines` materializes large signed
  frame-slot offsets through an AArch64 scratch register.
- Nearby but not owned by this extraction: `register_indirect_address` is a
  generic `[reg, #offset]` formatter used by variadic helpers; keep it with the
  existing memory/variadic surface unless a later packet deliberately extracts a
  separate generic address-formatting owner.

Required call sites after extraction:

- `memory.cpp` keeps using all extracted helpers internally, including prepared
  load offsets, store-local publication planning, fixed formal publication, and
  scalar stack-home loads/stores.
- Direct frame-slot address/base consumers need the new header if they stop
  receiving these declarations through `memory.hpp`: `alu.cpp`, `calls.cpp`,
  `cast_ops.cpp`, `dispatch.cpp`, `dispatch_edge_copies.cpp`,
  `dispatch_publication.cpp`, `dispatch_value_materialization.cpp`,
  `f128.cpp`, `fp_value_materialization.cpp`, `globals.cpp`,
  `prepared_value_home_materialization.cpp`, and `variadic.cpp`.
- `comparison.cpp` currently calls `find_frame_slot` directly and also receives
  `fixed_slots_use_frame_pointer` through hook setup; it needs either the new
  header or a deliberate switch to `prepare::find_frame_slot_by_id`.
- `prepared_value_home_materialization.cpp` currently uses `find_frame_slot`,
  `frame_slot_address`, and `fixed_slots_use_frame_pointer`; it is a useful
  compile canary for header completeness.
- `f128.cpp` currently uses both `materialize_frame_slot_memory_address_lines`
  and `make_prepared_frame_slot_memory_operand`; it should include the new owner
  header directly after extraction.
- `variadic.cpp` currently uses `fixed_slots_use_frame_pointer`, but its
  `va_list` field-address helpers are separate and should not move as part of
  this idea.

F128 and variadic touchpoints:

- f128: `f128_printable_memory_address` uses
  `materialize_frame_slot_memory_address_lines` for frame-slot offsets that are
  not directly encodable, and `f128_memory_backed_carrier_memory` uses
  `make_prepared_frame_slot_memory_operand` for memory-backed carrier records.
- f128 transport lowering inside `memory.cpp` depends on prepared frame-slot
  stack offsets and should keep using the extracted helper surface, but the
  packet should not move f128 transport ownership itself.
- variadic: `emit_prepared_va_list_field_carrier_to_register` passes
  `fixed_slots_use_frame_pointer(context.function)` into prepared home
  materialization. `prepared_va_list_field_address`, cursor updates, field-copy
  lowering, and `register_indirect_address` formatting remain variadic/memory
  concerns outside the frame-slot owner.

Effect of recently exported prealloc lookups:

- `prepare::find_frame_slot_by_id` and `prepare::find_stack_object_by_id` now
  exist in `src/backend/prealloc/prepared_lookups.hpp`; Step 2 should not keep
  hand-written scan loops as new authority. The smallest compatible route is to
  either replace local `find_frame_slot`/`find_stack_object` call sites with the
  exported `prepare::..._by_id` names, or keep thin AArch64 owner wrappers that
  forward to those exported functions while preserving the current call-site
  spelling. Do not create a second independent lookup implementation.

## Suggested Next

Delegate Step 2 as a code-changing extraction packet: create the
`frame_slot_address` owner, move or forward the mapped helper declarations out
of `memory.hpp`, update only required includes/call sites, add the new source to
`src/backend/CMakeLists.txt`, and preserve generated behavior.

Narrow proof command for Step 2:

```sh
cmake --build build --target c4c_backend_tests && ctest --test-dir build -R 'backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|backend_codegen_route_aarch64_(variadic_aggregate_overflow_byte_copy|dynamic_stack_fixed_slot_uses_fp_anchor)|backend_prepare_lookup_helper' --output-on-failure | tee test_after.log
```

## Watchouts

- Keep the route AArch64-memory-local; do not publish generic BIR, prealloc,
  storage-plan, register-spelling, scratch-selection, or opcode authority.
- Do not fold `va_list` field addressing, cursor updates, dispatch
  publication, edge-copy, or prepared-wrapper work into this idea.
- Do not claim progress through helper movement, renames, or expectation
  rewrites without proving equivalent generated records and diagnostics.
- `memory_store_retargeting.cpp` already includes `prepared_lookups.hpp` and
  uses `prepare::find_frame_slot_by_id` / `prepare::find_stack_object_by_id`;
  keep that file out of Step 2 unless include cleanup is strictly required.
- `memory.cpp` currently includes `constant_materialization.hpp` for
  `materialize_integer_constant_lines`; if
  `materialize_frame_slot_memory_address_lines` moves, the new source needs that
  dependency rather than relying on `memory.cpp`.
- Avoid moving `MemoryOperand` ownership or `memory_address`; the f128 helper
  only needs the frame-slot materialization API, not a broader memory split.

## Proof

Not run; the delegated packet explicitly required no build for this mapping-only
audit and forbade touching logs. No `test_after.log` was created or modified.
