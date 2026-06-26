Status: Active
Source Idea Path: ideas/open/389_rv64_va_start_destination_va_list_address_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Backend Coverage / Implement Narrow Destination Address Publication Follow-up

# Current Packet

## Just Finished

Steps 3-4 follow-up tightened the narrow RV64 `va_start` destination
`va_list` address materialization route so malformed prepared homes fail
closed when the destination-address GPR aliases the overflow-area scratch
register.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Implementation:

- `fragment_for_prepared_variadic_va_start` now receives the current
  `stack_frame_bytes`, resolves `homes->destination_va_list` through
  `prepared_stack_slot_home_offset`, and emits `addi dst_addr_gpr, sp,
  dst_slot_offset` before computing/storing the overflow-area pointer.
- `rv64_variadic_va_start_materialization_diagnostic` now guards the route
  with the Step 2 facts: matching prepared `VaStart` helper homes,
  destination `va_list` in a supported prepared stack-slot home, destination
  address in a prepared GPR home, supported va_list layout size, supported
  overflow-area field, and supported overflow-area base stack offset.
- The same diagnostic now rejects destination-address homes that resolve to the
  same GPR as the fixed overflow-area scratch register (`t1` / x6), avoiding a
  sequence that would overwrite the materialized destination address before
  the store.
- The implementation stays semantic: it does not hard-code `va-arg-13.c`,
  function `test`, `%lv.state.8`, register `s1`, slot #13, or offset 72.

Focused coverage:

- The RV64 object-emission `va_start` fixture now models the prepared
  destination `va_list` as an address-exposed stack-slot home with a separate
  destination-address GPR home.
- The positive test proves the emitted object materializes the destination
  address before the overflow-area field store:
  `addi a1,sp,72`; `addi t1,sp,64`; `sd t1,0(a1)`.
- New fail-closed coverage rejects malformed destination homes:
  destination `va_list` not in a supported stack-slot home, and destination
  address not in a prepared GPR home.
- The malformed-home coverage now also rejects a prepared destination-address
  GPR of `t1`, matching the current fixed scratch register used for the
  overflow-area pointer.

## Suggested Next

Execute Step 5: rerun `tests/c/external/gcc_torture/src/va-arg-13.c` through
the RV64 gcc-torture object runner and record whether it now passes or reaches
the next compile/link/runtime boundary.

## Watchouts

- Step 5 should be evidence-only unless delegated otherwise.
- Keep idea 386 frame-slot-address call arguments, idea 387 same-module sret,
  and idea 388 `va_end` routes closed.
- If the representative rerun exposes a later boundary, identify the likely
  owner instead of implementing another fix in the Step 5 packet.

## Proof

Ran the supervisor-selected Steps 3-4 follow-up proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```

Result: passed.

Proof log: `test_after.log`
