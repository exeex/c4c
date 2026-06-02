Status: Active
Source Idea Path: ideas/open/89_aarch64_memory_store_retargeting_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the Existing Retargeting Boundary

# Current Packet

## Just Finished

Step 1 boundary map is complete. AST-backed queries plus targeted reads found
the retargeting cluster in `memory.cpp`/`memory.hpp`, with no implementation
edits.

Chosen owner shape:

- Add `src/backend/mir/aarch64/codegen/memory_store_retargeting.hpp`.
- Add `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp`.
- Keep the owner under `c4c::backend::aarch64::codegen`; include it from
  `memory.cpp`.
- Remove the six retargeting declarations from `memory.hpp` after moving them
  to the new owner header unless another existing AArch64 memory consumer still
  needs them there. The stack-layout/local-address helpers are private today
  and should remain private to the new owner implementation/header surface.

Helpers to move:

- Pointer store-value/address retargeting:
  `store_local_uses_pointer_value_address`,
  `retarget_pointer_store_value_to_materialized_address`,
  `retarget_store_address_to_materialized_pointer`.
- Emitted-scalar store reuse:
  `prepared_or_emitted_store_value_register`,
  `retarget_pointer_store_value_to_emitted_scalar`,
  `retarget_store_local_value_to_emitted_scalar`.
- Local-address store-value rewrite:
  `find_prepared_local_address_store_value`,
  `rewrite_local_address_store_value`.
- Stack-layout application:
  `apply_stack_layout_to_memory_record`.

Required call sites found:

- `lower_memory_instruction` calls `apply_stack_layout_to_memory_record` after
  selected memory-record construction and before frame-pointer policy.
- `lower_store_local_value_publication` calls
  `store_local_uses_pointer_value_address` and
  `prepared_or_emitted_store_value_register` to avoid redundant source
  publication.
- `retarget_store_local_value_to_emitted_scalar` calls
  `store_local_uses_pointer_value_address` and
  `prepared_or_emitted_store_value_register`.
- `apply_stack_layout_to_memory_record` calls
  `find_prepared_local_address_store_value`,
  `resolve_frame_slot_memory_offset`, and
  `rewrite_local_address_store_value`.
- No direct helper calls were found in
  `backend_aarch64_instruction_dispatch_test.cpp` or
  `backend_aarch64_prepared_memory_operand_records_test.cpp`; those tests cover
  this boundary through memory lowering behavior.

Retained owner boundaries:

- Keep `memory.cpp` responsible for selecting memory records, diagnostics,
  lower-memory entry points, store publication planning, edge-copy behavior,
  prepared-wrapper contraction, and frame-pointer policy.
- The new owner may consume prepared pointer facts, stack-layout facts,
  value-home lookups, emitted scalar state, and register comparison/construction
  helpers. It must not recreate generic storage, register, diagnostic,
  dispatch, BIR, publication, or stack-layout authority.
- Preserve the current lookup/order semantics: emitted scalar before prepared
  home in `prepared_or_emitted_store_value_register`, no-op returns when the
  record/store shape is outside the retargeting contract, and fail-closed
  `false` only for missing prepared frame-slot/stack-layout authority.

## Suggested Next

Delegate Step 2 to an executor. Suggested packet: create
`memory_store_retargeting.hpp/.cpp`, move only the six pointer/emitted-scalar
helpers first, include the new header from `memory.cpp`, update build
registration, and keep memory lowering call sites explicit.

Suggested narrow proof for that implementation slice:

```bash
cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure
```

## Watchouts

- Keep the route memory-local; do not publish generic storage, register,
  diagnostic, dispatch, or BIR authority.
- Do not fold store publication, edge-copy, or prepared-wrapper work into this
  idea.
- Do not claim progress through expectation downgrades or helper renames alone.
- `apply_stack_layout_to_memory_record` currently depends on the private
  `resolve_frame_slot_memory_offset` helper in `memory.cpp`; Step 2 can avoid
  that dependency by moving only pointer/emitted-scalar helpers first.
- Step 3 must decide whether to move `resolve_frame_slot_memory_offset` with
  stack-layout application or leave stack-layout application in `memory.cpp`.
- Relevant focused behavioral tests include
  `pointer_value_store_conversion_preserves_prepared_and_bir_facts`,
  `pointer_value_store_combines_selected_address_and_member_offsets`,
  `frame_slot_store_prints_local_address_source_through_reserved_scratch`,
  `block_dispatch_lowers_materialized_pointer_address_store_writeback`,
  `block_dispatch_lowers_stack_homed_pointer_value_store_writeback`, and
  `block_dispatch_publishes_stack_homed_global_load_select_before_local_store`.

## Proof

Ran the supervisor-selected boundary-map proof. It is sufficient for this
no-implementation Step 1 packet and preserves `test_after.log`.

```bash
printf 'Boundary-map Step 1; no implementation tests required.\n' > test_after.log && git diff --name-only >> test_after.log && if git diff --name-only | rg -q '^src/backend/mir/aarch64/codegen/memory\.(cpp|hpp)$|^plan\.md$|^ideas/'; then printf 'ERROR: non-todo file changed during boundary-map packet.\n' >> test_after.log; exit 1; fi
```
