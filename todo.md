Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Memory Ownership Surfaces

# Current Packet

## Just Finished

Step 1 inspected the current AArch64 memory ownership surfaces and mapped the
behavior-preserving extraction targets for `memory.cpp`/`memory.hpp`.

Concrete extraction targets:

- Create `src/backend/mir/aarch64/codegen/memory.hpp` for memory-family public
  declarations currently exposed from `instruction.hpp`: `MemoryInstructionRecord`,
  `PreparedMemoryOperandRecordResult`, `PreparedMemoryInstructionRecordResult`,
  `make_prepared_memory_operand_record` overloads for local/global loads and
  stores, `make_prepared_frame_slot_load_memory_instruction_record`,
  `make_prepared_store_memory_instruction_record` overloads, and the
  memory-transport helpers that consume `std::optional<MemoryOperand>`.
- Create `src/backend/mir/aarch64/codegen/memory.cpp` for the memory-specific
  bodies currently in `instruction.cpp`: `prepared_memory_operand_record_error_name`,
  `memory_instruction_kind_name`, `memory_base_kind_name`,
  `memory_operand_support_kind_name`, `make_memory_operand`,
  `make_memory_instruction`, `make_spill_reload_instruction` only if the
  spill/reload record remains memory-family selected-node ownership, the
  prepared memory operand validators, `make_memory_record_from_prepared_access`,
  `validate_memory_base_identity`, `validate_memory_instruction_facts`,
  `apply_load_identity`, `apply_store_identity`, ordinary load/store record
  construction, and the i128/f128 memory transport record construction portions.
- Move from `dispatch.cpp` into memory-owned lowering helpers while leaving a
  dispatch route behind: `memory_error_message`, the body of
  `lower_memory_instruction`, the memory-specific body of
  `lower_f128_transport_instruction`, and address-fetching/diagnostic code that
  only exists to build `MemoryInstructionRecord` or f128 memory transports.
- Move from `machine_printer.cpp` into memory-owned printer helpers while
  leaving `print_machine_instruction_line_payloads` as the variant dispatcher:
  `memory_address`, `print_memory`, and the memory-address portions used by
  `print_spill_reload`, `print_i128_transport`, `print_f128_transport`,
  inline-asm memory operands, and vector-load intrinsic memory spelling.
- Keep `memory.md` as a reconciliation checklist until the compiled owner
  covers its durable behavior: direct frame-slot and pointer-value memory,
  prepared address facts, byte offset/size/align/address-space/volatile facts,
  typed load/store register spelling from selected records, F128 load/store
  helper routing, and selected-node side effects.

## Suggested Next

Delegate Step 2 to create `memory.hpp`/`memory.cpp` and first move the
prepared memory operand plus ordinary load/store selected-node construction
cluster out of `instruction.hpp`/`instruction.cpp`, leaving broad instruction
core as the family-neutral variant and record container surface.

## Watchouts

- Do not redistribute other AArch64 markdown shards in this route.
- Preserve memory behavior; do not downgrade tests or weaken supported-path
  contracts to claim progress.
- Keep these family-neutral boundaries outside memory ownership: `InstructionRecord`
  and its variant payload, generic `OperandRecord` and `RegisterOperand`
  primitives, `MachineOpcode`/mnemonic enum surfaces shared across families,
  `BlockLoweringContext`, module diagnostics plumbing, final instruction
  variant dispatch in `dispatch.cpp`, final printer variant dispatch in
  `machine_printer.cpp`, ABI register spelling primitives, and allocation,
  call/return, frame, and reserved-scratch contracts.
- Keep broad owners as routers only: `instruction.cpp` may retain generic
  record/operand helpers and cross-family enum naming, `dispatch.cpp` may choose
  the memory route, and `machine_printer.cpp` may dispatch by payload variant.
  Memory-family bodies should move to `memory.cpp`/`memory.hpp`.
- Carrier gaps remain blockers for expansion, not permission for local emitter
  recovery: legacy over-aligned alloca recomputation, dynamic stack pointer
  adjustment, bytewise memcpy scratch/loop ownership, global loads, F128
  memory-backed carriers, and arbitrary address materialization need prepared
  carriers or existing shared contracts before behavior can grow.
- Current ordinary memory lowering only selects local loads through frame slots,
  local stores through frame slots or pointer values, and global stores through
  prepared memory operands. Do not claim unsupported local pointer loads,
  global loads, over-aligned alloca, dynamic stack, or memcpy as completed by
  extraction alone.
- Spill/reload address spelling uses `MemoryOperand`, but spill/reload operation
  creation remains governed by allocation-result and `module::SpillReloadRecord`
  authority. Do not create spill/reload or scratch policy inside memory owner.
- Do not delete `memory.md` until its durable behavior and risks are
  reconciled into compiled memory ownership.

## Proof

Read-only inspection packet; no build or tests run. Future code proof selected
by supervisor: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
