Status: Active
Source Idea Path: ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Prepared Frame And Symbol Memory Bases

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: tightened direct AArch64 memory operand
record vocabulary so record-only memory operands can preserve prepared memory
facts without selecting load/store instructions.

Concrete work completed:
- Added `MemoryOperandSupportKind` with explicit `Prepared` and
  `DeferredUnsupported` states plus
  `memory_operand_support_kind_name(...)` diagnostics.
- Extended `MemoryOperand` with prepared memory-access identity:
  `function_name`, `block_label`, `instruction_index`, optional result
  `PreparedValueId` / `ValueNameId`, and optional stored-value
  `PreparedValueId` / `ValueNameId`.
- Preserved existing base facts and added structured string-symbol identity
  with `string_symbol_name` alongside existing `string_name`.
- Kept pointer memory bases as name/id slots (`pointer_value_name` and
  `pointer_value_id`) so later conversion can fill ids only from structured
  prepared value-location facts.
- Added `backend_aarch64_memory_operand_records` for direct frame-slot,
  symbol, string, pointer, deferred, and memory-instruction record behavior.
- Updated `backend_aarch64_target_operand_records` to cover the new access
  identity fields on the existing memory operand surface.

## Suggested Next

Execute Step 3 from `plan.md`: add conversion helpers that consume structured
BIR memory instructions plus matching `PreparedAddressing` /
`PreparedMemoryAccess` facts for frame-slot and global-symbol bases.

## Watchouts

- Keep this plan record-only; do not add load/store selection, assembly,
  encoding, object output, memory emission, calls, or returns.
- Preserve volatility and address-space facts from prepared input; do not
  invent target-local defaults.
- Step 2 intentionally added vocabulary and direct record proof only; no
  prepared/BIR conversion helpers were added yet.
- Step 3 should start with frame-slot and global-symbol bases. Pointer-value
  and string-constant conversion belong to Step 4 unless the supervisor
  explicitly narrows otherwise.
- Do not parse rendered global, slot, string, or value names. Frame and symbol
  conversion should use structured BIR ids plus prepared addressing facts, and
  fail closed when facts are missing or mismatched.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with `backend_aarch64_memory_operand_records`
included and green: 129 tests passed, 0 failed; 12 disabled MIR trace tests
were not run. Proof log path: `test_after.log`.
