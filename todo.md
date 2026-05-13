Status: Active
Source Idea Path: ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Prepared Pointer And String Memory Bases

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: added prepared/BIR-to-target memory operand
conversion for frame-slot and global-symbol bases only.

Concrete work completed:
- Added `PreparedMemoryOperandRecordError`,
  `PreparedMemoryOperandRecordResult`, and
  `prepared_memory_operand_record_error_name(...)`.
- Added `make_prepared_memory_operand_record(...)` overloads for
  `bir::LoadLocalInst`, `bir::StoreLocalInst`, `bir::LoadGlobalInst`, and
  `bir::StoreGlobalInst`.
- Conversion now consumes structured BIR memory instructions plus matching
  `PreparedAddressingFunction` / `PreparedMemoryAccess` facts keyed by
  function, block label, and instruction index.
- Frame-slot conversion preserves prepared frame-slot id, function/block/
  instruction identity, result or stored value name/id where available, byte
  offset, size, alignment, volatility, address space, and base-plus-offset
  eligibility.
- Global-symbol conversion preserves structured `LinkNameId` symbol identity
  and fails closed on mismatched or missing structured symbol ids.
- Conversion now validates structured BIR memory address facts against prepared
  access/address facts for byte offset, available size, available alignment,
  volatility, and address space before accepting the record.
- Instructions without structured BIR addresses are accepted only for facts
  carried by the legacy instruction fields: byte offset, alignment when
  present, default address space, non-volatile access, and no prepared size.
- Added `backend_aarch64_prepared_memory_operand_records` coverage for
  successful frame-slot load and global-symbol store conversion plus guard
  cases for missing prepared access, unsupported pointer base, result/store
  mismatch, symbol mismatch, BIR/prepared byte-offset mismatch, and
  BIR/prepared address-space mismatch.

## Suggested Next

Execute Step 4 from `plan.md`: extend prepared memory operand conversion to
pointer-value and string-constant bases where structured prepared/BIR facts
permit it.

## Watchouts

- Keep this plan record-only; do not add load/store selection, assembly,
  encoding, object output, memory emission, calls, or returns.
- Preserve volatility and address-space facts from prepared input; do not
  invent target-local defaults.
- Step 3 intentionally rejects pointer-value and string-constant prepared
  bases; those are the Step 4 target.
- Pointer-value conversion should join `PreparedAddress::pointer_value_name`
  to prepared value-location facts for `PreparedValueId`; fail closed if the
  join is missing or ambiguous.
- String-constant conversion should preserve structured string identity without
  parsing rendered string labels. Prepared string constants currently surface
  as `PreparedAddress::symbol_name` / `LinkNameId`.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with
`backend_aarch64_prepared_memory_operand_records` included and green: 130
tests passed, 0 failed; 12 disabled MIR trace tests were not run. Proof log
path: `test_after.log`.
