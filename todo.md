Status: Active
Source Idea Path: ideas/open/365_rv64_va_start_overflow_base_state_production.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define the RV64 Prepared Overflow-Area Contract

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for
`ideas/open/365_rv64_va_start_overflow_base_state_production.md`.

- Added an RV64 storage-authority producer in
  `src/backend/prealloc/variadic_entry_plans.cpp` for helper-bearing variadic
  entries. It creates or reuses a zero-sized, 8-byte-aligned prepared frame
  slot named `__rv64_variadic_overflow_area_base` with source kind
  `rv64_variadic_overflow_area_base`.
- RV64 helper-bearing entries now publish
  `overflow_area.base_slot_id` and `overflow_area.base_stack_offset_bytes` from
  that prepared frame slot, then clear the matching missing facts. The offset
  contract is the non-negative prepared frame-slot offset from the stable stack
  base used by object emission.
- Helper-free RV64 variadic entries were left without this storage authority;
  existing RV64 object admission still rejects helper-free variadic entries
  unless a separate explicit supported runtime contract exists.
- Focused BIR contract coverage now asserts the RV64 overflow base slot id,
  source kind, zero size, 8-byte alignment, and that
  `base_stack_offset_bytes` equals the prepared frame slot offset. Prepared
  printer coverage now asserts the RV64 helper-family printable overflow-area
  base slot and offset.

## Suggested Next

Step 3 packet: run the representative RV64 `va_start` path through normal
preparation/object emission to confirm the new producer unblocks helper-bearing
`va_start` without weakening explicit prepared-fact gating. Keep object
emission edits out of scope unless the produced facts expose a narrow consumer
contract mismatch.

Suggested narrow proof command remains:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure
```

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen idea-364 object-emission helper lowering except for narrow
  consumer adjustments required by a finalized producer contract.
- Do not broaden into external variadic call lowering, general parameter-home
  expansion, scalar `va_arg`, `va_copy`, or aggregate `va_arg`.
- `base_stack_offset_bytes` remains a `std::size_t`; Step 2 defines the RV64
  producer convention as a non-negative prepared frame-slot offset from the
  stable stack base. The existing RV64 consumer still enforces signed 12-bit
  immediate materializability separately.
- This packet did not run the representative `src/920908-1.c` allowlist case;
  Step 4 owns that broader proof unless the supervisor chooses to fold it into
  the next packet.

## Proof

Supervisor-selected proof passed and is preserved in `test_after.log`.

Command run exactly:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: build succeeded; `backend_prepare_frame_stack_call_contract`,
`backend_prepared_printer`, and `backend_riscv_object_emission` all passed.
