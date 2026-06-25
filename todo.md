Status: Active
Source Idea Path: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Admit the First Semantic Address Form

# Current Packet

## Just Finished

Completed Plan Step 2 for prepared frame-slot base-plus-subobject-offset
scalar local memory:

- RV64 object emission now uses the same `prepared_frame_slot_absolute_offset`
  helper for local-memory diagnostics that the lowering path uses for emitted
  loads/stores, so final offset checks include
  `slot.offset_bytes + access.address.byte_offset`.
- Focused object-emission coverage proves an `i32` local store and load whose
  frame slot starts at stack offset `8` and whose prepared access has
  `byte_offset = 4`; the emitted RV64 stack immediates are `12`.
- Fail-closed coverage preserves the local-memory diagnostic for missing slot
  id, pointer-value base, negative offset, final offset outside the frame,
  oversized alignment, volatile access, non-default address space, and
  unsupported FP scalar width.

## Suggested Next

Delegate Plan Step 3 to rerun the three idea-368 representatives through the
RV64 gcc_torture backend runner and classify the next owner for each:

- `src/20000217-1.c`
- `src/20000121-1.c`
- `src/va-arg-13.c`

Record which cases pass, which advance to pointer-value local memory,
frame-slot address call-argument materialization, aggregate `va_arg` helper
lowering, byval homes, or terminator work, and whether any in-scope
frame-slot base-plus-offset local-memory diagnostic remains.

## Watchouts

- Keep idea 354 open and inactive until child ideas 368-371 close or are
  intentionally superseded.
- Do not implement aggregate `va_arg`, byval homes, or terminator lowering from
  this plan.
- Do not hard-code representative offsets or source-case names; derive address
  facts from prepared metadata.
- Put analysis logs under `build/agent_state/`, not root-level canonical logs.
- Pointer-value local accesses through parameters were intentionally left
  unsupported; they should remain outside idea 368 unless the supervisor
  creates a separate pointer-memory owner.
- Step 3 may still expose frame-slot address call-argument materialization,
  aggregate `va_arg`, byval, or terminator blockers; route those to the
  existing child ideas instead of expanding this packet.

## Proof

Supervisor-selected proof passed, 3/3 tests, with output preserved in
`test_after.log`:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```
