Status: Active
Source Idea Path: ideas/open/143_stack_layout_id_lookup_helpers_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Helper Declarations, Definitions, And Consumers

# Current Packet

## Just Finished

Step 1 inspected the stack-layout id lookup helper declarations,
definitions, consumers, include dependencies, and stack-layout source-file
conventions.

Current helper map:

- Public declarations:
  - `src/backend/prealloc/prepared_lookups.hpp:21` declares
    `find_frame_slot_by_id(const PreparedStackLayout&, PreparedFrameSlotId)`.
  - `src/backend/prealloc/prepared_lookups.hpp:25` declares
    `find_stack_object_by_id(const PreparedStackLayout&, PreparedObjectId)`.
- Public definitions:
  - `src/backend/prealloc/prepared_lookups.cpp:1045` scans
    `PreparedStackLayout::frame_slots` by `PreparedFrameSlot::slot_id`.
  - `src/backend/prealloc/prepared_lookups.cpp:1056` scans
    `PreparedStackLayout::objects` by `PreparedStackObject::object_id`.
- Private duplicate:
  - `src/backend/prealloc/regalloc/value_homes.cpp:74` has an anonymous
    namespace-only `find_frame_slot_by_id` duplicate used by
    `find_stack_passed_f128_formal_local_home` at line 101. This is not a
    public consumer but should be folded to the stack-layout-owned helper in
    Step 2 if that file can include `stack_layout/stack_layout.hpp` without
    widening behavior.
- Direct public-helper callers:
  - `src/backend/prealloc/prepared_lookups.cpp:641`
    `prepared_frame_address_object_is_addressable` calls
    `find_stack_object_by_id`.
  - `src/backend/prealloc/prepared_lookups.cpp:654`
    `prepared_frame_slot_for_access` calls `find_frame_slot_by_id`.
  - `src/backend/prealloc/prepared_lookups.cpp:1935`
    `find_indexed_prepared_frame_address_offset_for_value` calls
    `find_frame_slot_by_id`.
  - `src/backend/prealloc/prepared_lookups.cpp:2010`
    `find_indexed_prepared_frame_address_offset_for_value_id` calls
    `find_frame_slot_by_id`.
  - `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp:39`
    `make_frame_slot_operand_from_stack_slot` calls
    `prepare::find_stack_object_by_id`.
  - `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp:87`
    `find_prepared_local_address_store_value` calls
    `prepare::find_frame_slot_by_id`.
  - `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp:146`
    `resolve_frame_slot_memory_offset` calls `prepare::find_frame_slot_by_id`.
  - `src/backend/mir/aarch64/codegen/comparison.cpp:1396`
    local wrapper `find_frame_slot` calls `prepare::find_frame_slot_by_id`.
  - `src/backend/mir/aarch64/codegen/frame_slot_address.cpp:67`
    local wrapper `find_frame_slot` calls `prepare::find_frame_slot_by_id`.
  - `src/backend/mir/aarch64/codegen/frame_slot_address.cpp:73`
    local wrapper `find_stack_object` calls
    `prepare::find_stack_object_by_id`.
  - `src/backend/mir/aarch64/codegen/calls.cpp:5769`
    `find_prepared_frame_byte_stored_value` calls
    `prepare::find_frame_slot_by_id`.
  - `src/backend/mir/aarch64/codegen/calls.cpp:5829`
    `find_prepared_frame_stored_value` calls
    `prepare::find_frame_slot_by_id`.
  - `src/backend/mir/aarch64/codegen/calls.cpp:5879`
    `find_prepared_frame_containing_byte_stored_value` calls
    `prepare::find_frame_slot_by_id`.

Include dependency notes:

- `prepared_lookups.hpp` already includes `stack_layout/stack_layout.hpp`, so
  moving declarations there should not require new type forward declarations.
- `memory_store_retargeting.cpp` already includes both
  `prepared_lookups.hpp` and `stack_layout/stack_layout.hpp`; keep
  `prepared_lookups.hpp` for residual APIs such as
  `resolve_prepared_value_name_id` and
  `find_indexed_prepared_frame_address_offset_for_value_id`.
- `frame_slot_address.cpp` currently includes `prepared_lookups.hpp` for both
  stack id helpers and residual prepared addressing lookups
  (`find_prepared_addressing`, `find_prepared_memory_access`); it should add
  the stack-layout owner but keep `prepared_lookups.hpp`.
- `comparison.cpp` uses many residual prepared lookup APIs, so add the
  stack-layout owner for the id helper and keep `prepared_lookups.hpp`.
- `calls.cpp` uses many residual prepared lookup APIs and currently reaches
  the public declarations transitively through its local include graph rather
  than directly including `prepared_lookups.hpp`; Step 2 should add the
  stack-layout owner directly for the id helper without relying on transitive
  exposure.
- `regalloc/value_homes.cpp` already has `PreparedStackLayout` visibility
  through local prealloc headers but owns a private duplicate scan; Step 2
  should replace that duplicate with the public stack-layout-owned helper if
  the include stays narrow.

Stack-layout source-file conventions:

- `src/backend/prealloc/stack_layout/README.md` says the directory owns active
  C++ stack-frame layout implementation and is compiled through the prealloc
  source glob.
- Existing `.cpp` files are phase-specific: `analysis.cpp`,
  `alloca_coalescing.cpp`, `copy_coalescing.cpp`, `inline_asm.cpp`,
  `regalloc_helpers.cpp`, `slot_assignment.cpp`, and `coordinator.cpp`.
- `coordinator.cpp` is the only stack-layout `.cpp` already in namespace
  `c4c::backend::prepare`; the others use
  `c4c::backend::prepare::stack_layout` for construction internals.

Chosen Step 2 implementation home:

- Declare the helpers in
  `src/backend/prealloc/stack_layout/stack_layout.hpp` in namespace
  `c4c::backend::prepare`.
- Implement them in a new
  `src/backend/prealloc/stack_layout/lookups.cpp` file, also in namespace
  `c4c::backend::prepare`. This is narrower than adding unrelated pure lookup
  scans to construction-phase files and avoids overloading `coordinator.cpp`
  with non-orchestration helpers.

## Suggested Next

Execute Step 2 from `plan.md`: move the two public declarations from
`prepared_lookups.hpp` to `stack_layout/stack_layout.hpp`, move the public
definitions from `prepared_lookups.cpp` to new
`stack_layout/lookups.cpp`, remove the private duplicate in
`regalloc/value_homes.cpp` by using the stack-layout-owned helper, and update
direct consumers to include the stack-layout owner while preserving residual
`prepared_lookups.hpp` includes where still needed.

## Watchouts

- This active idea is an ownership move only; do not change stack layout,
  frame-address, addressing, memory retargeting, comparison, or AArch64
  wrapper behavior.
- Do not use this route for broad residual include cleanup; idea 149 owns that
  after narrower owner moves land.
- Keep `prepared_lookups.hpp` includes where a consumer still needs
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, or another
  residual prepared lookup API.
- Do not make stack id lookup target-specific.

## Proof

Inspection-only analysis slice. Per delegated proof, no build or tests were
run and no `test_after.log` was required.
