Status: Active
Source Idea Path: ideas/open/234_aarch64_memory_load_store_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Frame-Slot Load Nodes

# Current Packet

## Just Finished

Step 2 selected prepared frame-slot BIR loads into structured AArch64
`MemoryInstructionRecord` nodes without adding terminal load printer output.

Completed work:

- Added `MemoryInstructionRecord::result_register` and a
  `PreparedMemoryInstructionRecordResult` builder for frame-slot `LoadLocalInst`
  records.
- The new builder reuses `PreparedMemoryAccess` validation, requires a frame-slot
  base, resolves the result through prepared value homes plus storage-plan
  register authority, and fails closed with explicit
  `missing_result_value_home`, `missing_result_storage`,
  `unsupported_result_storage`, or `register_conversion_failed` errors.
- `make_memory_instruction` now emits a register definition for loads when the
  structured result register is present while preserving existing prepared-value
  fallback behavior for older hand-built records.
- `dispatch_prepared_block` now consumes prepared frame-slot loads, records the
  selected result register for later scalar/return consumers, preserves
  frame-slot id, byte offset, size, alignment, volatility, address space, and
  memory-read side effect, and reports memory-family diagnostics for missing
  prepared access or destination authority.
- Focused MIR tests cover direct record construction, dispatch selection from
  prepared facts, and missing load-destination storage diagnostics.

## Suggested Next

Step 3 implementation packet: select frame-slot store nodes by resolving the
stored source value through prepared value-home/storage authority into a
structured register operand on `MemoryInstructionRecord`, without terminal
store-printer expansion beyond existing behavior.

## Watchouts

- Terminal load printer output is still deferred; this packet only selected
  machine records and structured defs.
- The selected dispatch path is intentionally frame-slot loads only. Stores,
  pointer-value memory, and symbol/string memory access remain later packets.
- Global address materialization remains separate from global load/store
  lowering. Do not infer memory access policy from rendered names.
- Other memory instructions still fall through to the existing unsupported
  memory-family diagnostics until their packets add selected builders.

## Proof

Fresh backend subset proof passed and wrote `test_after.log`:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract)$') > test_after.log 2>&1`

Result: 3/3 focused backend MIR tests passed.
