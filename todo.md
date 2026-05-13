Status: Active
Source Idea Path: ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Memory Inputs And Existing Owners

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: inspected existing AArch64 memory record
owners, shared prepared memory facts, BIR memory inputs, and representative
backend fixtures before code changes.

Existing owner files:
- `src/backend/mir/aarch64/codegen/records.hpp` / `.cpp` own the target
  record-only memory operand surface: `MemoryBaseKind`, `MemoryOperand`,
  `MemoryInstructionRecord`, `make_memory_operand(...)`,
  `make_memory_instruction(...)`, and diagnostic names.
- `src/backend/mir/aarch64/codegen/memory.md` is legacy lowering reference
  only; it documents old load/store/address-materialization behavior and
  scratch registers, not an implementation owner for this plan.
- `src/backend/mir/aarch64/module/module.hpp` / `.cpp` own prepared/BIR
  snapshots such as `FrameSlotRecord`, data records, and generic prepared
  operand snapshots. They should remain snapshot owners, not target memory
  operand conversion owners.
- Shared preparation owns source memory facts in `src/backend/prealloc/`,
  especially `PreparedAddressing`, `PreparedAddressingFunction`,
  `PreparedMemoryAccess`, and `PreparedAddress`.

Usable prepared and BIR inputs:
- BIR `MemoryAddress` carries `BaseKind::{LocalSlot, GlobalSymbol,
  PointerValue, StringConstant}`, `base_value`, `byte_offset`, `size_bytes`,
  `align_bytes`, `address_space`, `is_volatile`, and structured ids where
  present (`base_link_name_id`, `base_slot_id`).
- BIR load/store inputs include `LoadLocalInst`, `StoreLocalInst`,
  `LoadGlobalInst`, and `StoreGlobalInst`; prepared builders already consume
  these instead of printed BIR.
- Prepared `PreparedMemoryAccess` carries `function_name`, `block_label`,
  `inst_index`, optional `result_value_name` / `stored_value_name`,
  `address_space`, `is_volatile`, and nested `PreparedAddress`.
- Prepared `PreparedAddress` carries `base_kind`, optional `frame_slot_id`,
  optional symbol `LinkNameId`, optional pointer `ValueNameId`, `byte_offset`,
  `size_bytes`, `align_bytes`, and `can_use_base_plus_offset`.
- `PreparedBirModule::addressing` plus
  `find_prepared_addressing(...)` /
  `find_prepared_memory_access(...)` are the lookup entry points for
  function/block/instruction-index addressing facts.

Unsupported or missing inputs to preserve in later packets:
- Existing `MemoryOperand` has no prepared memory-access identity fields
  (`function_name`, `block_label`, `instruction_index`, result/stored value
  ids/names). Step 2 should add or tighten those before conversion.
- Existing `MemoryOperand` has `string_name` as `TextId`, while prepared
  string-constant bases currently appear through `PreparedAddress::symbol_name`
  / `LinkNameId`; Step 2 should decide how to represent this without parsing
  rendered string labels.
- Prepared pointer-value addresses carry `pointer_value_name` but not a direct
  `PreparedValueId`; later conversion can fill `pointer_value_id` only by
  joining through prepared value-location facts, and must fail closed if that
  join is missing or ambiguous.
- `PreparedAddressBaseKind::None`, BIR `MemoryAddress::BaseKind::Label`, and
  atomics/vector/inline-asm memory effects are outside this slice and should
  fail closed or stay deferred.
- `PreparedMemoryAccess` does not itself carry a load/store kind; conversion
  should derive load/store shape from the matching structured BIR instruction
  plus result/stored value fields.

Representative fixture candidates:
- `tests/backend/backend_aarch64_target_operand_records_test.cpp` already
  checks direct `MemoryOperand` construction with frame-slot fields,
  volatility, and base-plus-offset.
- `tests/backend/backend_aarch64_target_instruction_records_test.cpp` already
  checks `MemoryInstructionRecord` as a record-only instruction payload.
- `tests/backend/backend_prepare_stack_layout_test.cpp` has direct prepared
  fixture coverage for frame-slot, global-symbol, string-constant, and
  pointer-value bases, including volatile and non-default address-space facts
  (`Fs`, `Gs`, `Tls`).
- `check_prepared_addressing_contract_activation()` in that test provides a
  compact hand-built prepared-addressing fixture for frame-slot and
  string-constant facts.

## Suggested Next

Execute Step 2 from `plan.md`: tighten the `codegen/records.hpp` memory
operand vocabulary so direct record construction can preserve prepared memory
access identity, base kind, frame-slot id, symbol/string identity,
pointer-value name/id slots, byte offset, size, alignment, volatility, and
address space without load/store selection.

## Watchouts

- Keep this plan record-only; do not add load/store selection, assembly,
  encoding, object output, memory emission, calls, or returns.
- Preserve volatility and address-space facts from prepared input; do not
  invent target-local defaults.
- Keep `module/` as the prepared/BIR snapshot owner. New target-local memory
  operand vocabulary and helpers should stay under `codegen/`.
- Do not parse rendered global, slot, string, or value names. If pointer ids or
  string identity cannot be obtained from structured prepared/BIR facts, fail
  closed and record the gap.

## Proof

No validation run. This was an inspection-only packet.
