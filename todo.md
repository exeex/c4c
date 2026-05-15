Status: Active
Source Idea Path: ideas/open/234_aarch64_memory_load_store_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select Frame-Slot And Pointer-Value Store Nodes

# Current Packet

## Just Finished

Step 3 selected prepared frame-slot and pointer-value BIR stores into structured
AArch64 `MemoryInstructionRecord` nodes without expanding terminal printer
behavior.

Completed work:

- Added prepared store machine-record builders for `StoreLocalInst` and
  `StoreGlobalInst`.
- The selected store builder reuses `PreparedMemoryAccess` validation, accepts
  only `FrameSlot` and `PointerValue` bases, resolves the stored source through
  prepared value homes plus storage-plan register authority, and places the
  source as a structured register operand on `MemoryInstructionRecord::value`.
- Store failures now diagnose explicit stored-source authority gaps through
  `missing_stored_value_home`, `missing_stored_storage`,
  `unsupported_stored_storage`, or `register_conversion_failed`.
- `dispatch_prepared_block` now consumes prepared frame-slot and pointer-value
  store facts, preserving base identity, byte offset, size, alignment,
  volatility, address space, and memory-write side effects.
- Unsupported symbol/global store bases are handled by the prepared store
  builder and reported as memory-family `unsupported_base` diagnostics instead
  of being selected or inferred from rendered names.
- Focused MIR tests cover direct frame-slot/pointer-value store record
  construction, dispatch selection, missing stored-source storage diagnostics,
  and unsupported global-symbol store diagnostics.

## Suggested Next

Step 4 implementation packet: add terminal AArch64 printer support for the
selected frame-slot load/store subset only, driven by structured
`MemoryInstructionRecord` fields. Keep global/symbol/string memory printing and
any unstructured base handling deferred.

## Watchouts

- Terminal load printer output is still deferred. Existing store printer
  behavior was not expanded in this packet.
- Selected memory coverage is currently frame-slot loads plus frame-slot and
  pointer-value stores. Pointer-value loads, global/symbol memory, and string
  memory remain later packets unless the plan-owner narrows them differently.
- Global address materialization remains separate from global load/store
  lowering. Do not infer memory access policy from rendered names.
- `StoreGlobalInst` can now reach explicit memory diagnostics for unsupported
  bases, but selected global memory access is still deferred.

## Proof

Fresh backend subset proof passed and wrote `test_after.log`:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract)$') > test_after.log 2>&1`

Result: 3/3 focused backend MIR tests passed.
