Status: Active
Source Idea Path: ideas/open/value-home-storage-interpretation-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Current AArch64 Decoding

# Current Packet

## Just Finished

Completed `Step 1: Inventory Current AArch64 Decoding` as a read-only audit.

Inspected:

- `src/backend/mir/aarch64/codegen/operands.hpp`
  - `OperandAuthority`, `ResolvedOperand`, and public helpers
    `resolve_value_operand`, `resolve_immediate_operand`,
    `resolve_label_operand`, `resolve_symbol_operand`.
- `src/backend/mir/aarch64/codegen/operands.cpp`
  - AST inventory via `c4c-clang-tool-ccdb function-signatures`.
  - Helper boundaries: `resolve_from_regalloc`, `resolve_from_storage_plan`,
    `resolve_from_value_home`, `make_register_operand`, `to_mir_register`,
    and the literal `resolve_*_operand` wrappers.
- Nearby AArch64 consumers:
  - `src/backend/mir/aarch64/codegen/alu.cpp`
  - `src/backend/mir/aarch64/codegen/comparison.cpp`
  - `src/backend/mir/aarch64/codegen/returns.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch.cpp`
- Existing prealloc surfaces:
  - `src/backend/prealloc/storage.hpp`
  - `src/backend/prealloc/storage_plans.hpp`
  - `src/backend/prealloc/storage_plans.cpp`
  - `src/backend/prealloc/regalloc.hpp`
  - `src/backend/prealloc/value_locations.hpp`
  - `src/backend/prealloc/regalloc/value_homes.hpp`
- x86 prepared reuse surface:
  - `src/backend/mir/x86/prepared/prepared.hpp`
  - `src/backend/mir/x86/prepared/operands.cpp`

Extraction boundary:

- Move target-neutral lookup and interpretation out of AArch64:
  - value-id lookup in `PreparedRegallocFunction::values`;
  - regalloc assigned-storage classification:
    `assigned_register` with `PreparedRegisterPlacement`, or
    `assigned_stack_slot` with slot id and byte offset;
  - storage-plan lookup by `PreparedValueId`;
  - storage-plan encoding classification:
    `Register`, `FrameSlot`, `Immediate`, `SymbolAddress`,
    `ComputedAddress`, `None`;
  - value-home lookup through `find_prepared_value_home`;
  - value-home kind classification:
    `StackSlot`, `RematerializableImmediate`, `Register`,
    `PointerBasePlusOffset`, `None`;
  - required-field checks for decoded forms, such as register placement for
    register storage, slot id for frame/stack storage, `immediate_i32` for
    i32 immediates, and `symbol_name` for symbols.
- Keep AArch64-local:
  - `abi::convert_prepared_register`, `abi::RegisterReference`, register-bank
    spelling, register view selection, and MIR physical-register construction;
  - construction of `mir::Operand`, `mir::Memory`, `mir::Immediate`, and
    `mir::Symbol`;
  - AArch64 diagnostics and exact unsupported-form wording;
  - frame-slot operand spelling details such as base/index/scale choice and
    final memory operand support flags in consumers;
  - immediate legality and scalar type/view checks in `alu.cpp`,
    `comparison.cpp`, and `returns.cpp`;
  - final operand roles derived from `OperandAuthority`.

Decoded forms prealloc should expose for Step 2:

- Register-backed prepared value:
  `PreparedValueId`, `ValueNameId`, authority/source, optional prepared class,
  `PreparedRegisterPlacement`, bank/class/width metadata where available.
- Stack/frame-backed prepared value:
  `PreparedValueId`, `ValueNameId`, authority/source,
  `PreparedFrameSlotId`, byte offset, optional size/align, and spill-slot
  distinction when sourced from a storage plan.
- Rematerializable i32 immediate:
  `PreparedValueId`, `ValueNameId`, authority/source, signed i32 payload.
- Symbol address:
  `PreparedValueId`, `ValueNameId`, authority/source, `LinkNameId`.
- Unsupported but explicitly decoded forms:
  computed address / pointer-base-plus-offset, missing register placement,
  missing slot id, missing immediate payload, missing symbol id, and no
  authority.

Prealloc API should return decoded facts, not MIR operands. AArch64 should map
those facts to `ResolvedOperand`; x86 can map the same facts to text/encoding
through `x86::prepared::Query`, which already provides `locations()` and can be
extended to expose storage-plan access from the prepared module.

## Suggested Next

Execute `Step 2: Add Prealloc Decoded Home/Storage API` by adding a
target-neutral decoded Prepared home/storage result type or helper in
`src/backend/prealloc`, then adapt no target code yet unless the supervisor
delegates it.

## Watchouts

- Keep prealloc limited to target-neutral Prepared home/storage interpretation.
- Keep target register spelling, addressing legality, immediate legality, and
  final operand construction in target code.
- Do not weaken unsupported-form tests or add named-case storage shortcuts.
- `resolve_from_regalloc` currently gives regalloc assignments priority over
  storage plans and value homes; preserve that precedence unless plan review
  explicitly changes the contract.
- `PreparedValueHomeKind::Register` is not enough for AArch64 operand spelling
  unless typed placement comes from regalloc or storage plans; do not make
  prealloc manufacture AArch64 register spelling from `register_name`.
- `ComputedAddress` and `PointerBasePlusOffset` should be decoded as
  unsupported-for-this-operand-form rather than collapsed into stack slots or
  symbols.

## Proof

Documentation/audit-only packet. No build or ctest was run because this slice
only inspected source files and updated `todo.md`; no implementation,
headers, tests, or generated outputs changed. No `test_after.log` update was
needed for this audit proof.
