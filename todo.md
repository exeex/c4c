Status: Active
Source Idea Path: ideas/open/234_aarch64_memory_load_store_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Memory And AArch64 Surfaces

# Current Packet

## Just Finished

Step 1 inspected the prepared memory and AArch64 memory-node surfaces.

Current carriers:

- Prepared memory facts are `prepare::PreparedMemoryAccess` records in
  `PreparedAddressingFunction::accesses`, keyed by function, block label, and
  BIR instruction index.
- Each prepared access carries load result or store source value name,
  address-space, volatility, and a `PreparedAddress` with base kind, frame-slot
  id, symbol/text identity, pointer-value identity, byte offset, size,
  alignment, and base-plus-offset eligibility.
- Stack layout already publishes prepared access facts for local/global
  load/store instructions across frame-slot, global-symbol, string-constant,
  and pointer-value bases when upstream facts are structured enough.
- AArch64 conversion already has `make_prepared_memory_operand_record`
  overloads for `LoadLocalInst`, `StoreLocalInst`, `LoadGlobalInst`, and
  `StoreGlobalInst`. These validate prepared/BIR fact agreement and fail closed
  through `PreparedMemoryOperandRecordError`.

Selected-memory gaps:

- `MemoryOperand` and `MemoryInstructionRecord` already preserve prepared
  address facts, result/stored prepared ids, value type, memory side effects,
  and `MachineOpcode::Load`/`Store`.
- Selection currently accepts only prepared `FrameSlot` and `PointerValue`
  bases. `Symbol`, `StringConstant`, `None`, and `Register` bases defer with
  `memory operand is outside the selected subset`.
- `dispatch_prepared_block` does not currently call a memory-lowering path for
  BIR load/store instructions. It tries calls, address materialization,
  scalars, then unsupported-instruction diagnostics.
- Load records can be selected in record tests when ids are filled manually,
  but there is no dispatch-owned path that resolves the load destination
  register from prepared value-home/storage authority.
- Store records can be selected manually when the stored value operand is
  already supplied, but dispatch does not yet build that source register from
  prepared value-home/storage authority.

Printer support and diagnostics:

- `print_memory` currently prints selected stores only when the stored value is
  a register operand and the address is a prepared base-plus-offset printable
  by `memory_address`.
- `memory_address` currently prints frame-slot and pointer-value register bases;
  it does not print symbol/string bases.
- `print_memory` explicitly rejects loads with
  `load node is missing a structured destination register operand`.
- Store printer diagnostics are explicit for missing stored-value operand,
  non-register stored value, unprepared/non-base-plus-offset address, missing
  printable address, and missing mnemonic.
- Existing tests already assert fail-closed prepared conversion errors such as
  missing prepared access, unsupported base, address fact mismatch, pointer-home
  gaps, ambiguity, string identity mismatch, result mismatch, and stored-value
  mismatch.

## Suggested Next

Step 2 implementation packet: select prepared frame-slot load nodes with
structured destination registers, without adding terminal load printer output.

Suggested packet:

`to_subagent: c4c-executor`
`Objective: Execute Step 2 by lowering prepared frame-slot BIR loads into selected AArch64 MemoryInstructionRecord nodes with structured destination register facts, without terminal load printer output.`
`Plan Step: Step 2: Select Frame-Slot Load Nodes`
`Owned Files: todo.md, src/backend/mir/aarch64/codegen/instruction.hpp, src/backend/mir/aarch64/codegen/instruction.cpp, src/backend/mir/aarch64/codegen/dispatch.cpp, tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp or tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
`Do Not Touch: plan.md, ideas/open/234_aarch64_memory_load_store_machine_nodes.md, terminal printer output paths except existing diagnostics/tests if needed, unrelated frontend/x86 files`
`Proof: (cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract)$') > test_after.log 2>&1`
`Done When: dispatch consumes prepared frame-slot load facts, resolves the load destination from prepared value-home/storage authority into a structured register on MemoryInstructionRecord, preserves frame-slot id/offset/size/alignment/volatility/address-space and memory-read side effect, and emits explicit diagnostics for missing prepared access or destination register authority.`

## Watchouts

- Do not infer memory address authority inside AArch64 target codegen.
- Keep global address materialization separate from global memory access.
- Do not revive archived scratch-register or accumulator conventions.
- Preserve explicit diagnostics for unsupported symbol/string or unprepared
  memory bases.
- Step 2 should not print loads yet; the current terminal printer intentionally
  rejects loads until selected records carry a structured destination register
  and Step 4 owns load/store printing.
- The first selected subset should stay frame-slot loads only. Pointer-value
  stores and broader store support belong to Step 3.

## Proof

Inspection-only packet. No build/tests were run, and `test_after.log` was not
created or modified.
