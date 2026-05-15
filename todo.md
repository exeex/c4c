Status: Active
Source Idea Path: ideas/open/234_aarch64_memory_load_store_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Print Structured Load/Store Subset

# Current Packet

## Just Finished

Step 4 printed the selected structured AArch64 memory subset from complete
`MemoryInstructionRecord` fields.

Completed work:

- `print_memory` now emits selected `ldr` records from
  `MemoryInstructionRecord::result_register` plus prepared memory address facts.
- Existing selected store printing remains field-driven through
  `MemoryInstructionRecord::value` and prepared memory address facts; the common
  printer now handles frame-slot stores and pointer-value stores with structured
  base registers.
- Pointer-value store selection now carries the pointer base register from
  prepared pointer value-home/storage authority, so printer output does not
  infer the base from rendered names.
- Missing load destination register, missing store source register, missing
  pointer base register, unprepared memory operands, and unprintable memory
  addresses still fail closed with explicit diagnostics.
- Symbol/string/global memory bases still do not print through fallback
  templates. Unsupported global memory remains diagnostic-only.
- Focused printer tests cover frame-slot load output, frame-slot store output,
  pointer-value store output, missing destination/source/address diagnostics,
  and unprepared memory rejection.

## Suggested Next

Step 5 validation packet: validate semantic coverage for the active plan's
supported memory subset: frame-slot load, frame-slot store, pointer-value store,
and unsupported/unprepared base diagnostics. Confirm global address
materialization remains separate from global memory access, and run the
supervisor-selected broader proof for close-quality confidence.

Pointer-value loads are outside the current active plan/source proof direction.
If they are needed next, frame them as a possible separate lifecycle initiative
for plan-owner review rather than as Step 5 follow-on work.

## Watchouts

- Terminal output currently covers selected frame-slot loads, frame-slot stores,
  and pointer-value stores only.
- Pointer-value loads, global/symbol memory, and string memory remain later
  packets unless the plan-owner narrows them differently.
- Global address materialization remains separate from global load/store
  lowering. Do not infer memory access policy from rendered names.
- `StoreGlobalInst` can now reach explicit memory diagnostics for unsupported
  bases, but selected global memory access is still deferred.

## Proof

Fresh backend subset proof passed and wrote `test_after.log`:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_aarch64_machine_printer)$') > test_after.log 2>&1`

Result: 4/4 focused backend MIR tests passed.
