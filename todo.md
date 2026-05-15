Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Global Address Surfaces

# Current Packet

## Just Finished

Step 1 inspection completed for AArch64 global address materialization surfaces.

Existing address-producing and symbol-backed surfaces found:

- BIR global address/value identity:
  `bir::Global` carries `link_name_id`, `is_extern`, `is_thread_local`, and
  initializer symbol identity; `bir::Value::pointer_symbol_link_name_id` plus
  `Value::named_symbol_pointer()` carries pointer values that denote globals or
  functions.
- BIR memory address carriers:
  `bir::MemoryAddress::BaseKind` already distinguishes `GlobalSymbol`,
  `PointerValue`, `Label`, and `StringConstant`, with `base_link_name_id`,
  `base_label_id`, `base_slot_id`, `AddressSpace::{Default,Fs,Gs,Tls}`,
  volatility, size, alignment, and byte offset.
- Prepared memory carriers:
  `prepare::PreparedAddressBaseKind` currently has `GlobalSymbol`,
  `PointerValue`, and `StringConstant` but not `Label`; `PreparedAddress`
  carries `symbol_name`, `pointer_value_name`, offset, size, alignment, and
  `can_use_base_plus_offset`; `PreparedMemoryAccess` preserves address space
  and volatility for load/store memory operands.
- AArch64 MIR memory carriers:
  `aarch64::codegen::MemoryBaseKind` has `Symbol`, `PointerValue`, and
  `StringConstant`; `MemoryOperand` carries `symbol_name`,
  `string_symbol_name`, `string_name`, `pointer_value_name`,
  `pointer_value_id`, offset, size, alignment, address space, and volatility.
- AArch64 selected-node/printer surfaces:
  `MachineOpcode` has load/store but no address-materialization opcode;
  `MemoryInstructionRecord` only models memory load/store. The printer only
  renders frame-slot and pointer-value base+offset memory addresses, and has no
  relocation-aware ADRP/ADD, GOT load, label-address, or TLS-address printer
  path.

Current gaps and unsupported diagnostics:

- AArch64 memory selection explicitly rejects `MemoryBaseKind::Symbol` and
  `MemoryBaseKind::StringConstant` via
  `memory operand is outside the selected subset`; only frame-slot and
  pointer-value memory bases select today.
- Label addresses exist in BIR (`MemoryAddress::BaseKind::Label`) and BIR
  printer tests, but the prepared address layer has no label base kind, so label
  address-producing facts are dropped before AArch64 MIR.
- TLS is currently only an `AddressSpace::Tls` fact on memory accesses or
  `bir::Global::is_thread_local`; there is no prepared/AArch64 address-kind
  carrier for TLS-relative global address materialization.
- GOT/direct relocation policy is missing from active prepared and AArch64 MIR
  records. The only detailed policy is archived in
  `src/backend/mir/aarch64/codegen/globals.md`, not in structured code
  carriers.
- String constants are modeled as symbol-backed memory bases
  (`StringConstant`) for accesses, not as standalone address materialization
  facts for a prepared result home.

## Suggested Next

Step 2 first implementation packet target: add a structured prepared address
materialization carrier beside, not inside, `PreparedMemoryAccess` in
`src/backend/prealloc/prealloc.hpp`, starting with direct global and string
constant address cases. It should carry address kind, `LinkNameId`/text
identity, byte offset, address space/TLS facts, and the prepared result value
home/name, while leaving load/store memory-access carriers unchanged.

## Watchouts

Keep memory load/store lowering separate from address-producing values. Do not
try to make `MemoryBaseKind::Symbol` selected as a proxy for address
materialization; the missing first carrier is a result-producing prepared
address fact, not a printable memory operand. Label support needs an explicit
prepared base kind or a separate address-materialization kind before AArch64 can
preserve `BlockLabelId`. GOT/direct/TLS policy must be structured and must not
come from rendered symbol spelling.

## Proof

Inspection-only packet; no build/test required and no `test_after.log` was
produced.
