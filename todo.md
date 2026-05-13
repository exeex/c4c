Status: Active
Source Idea Path: ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Volatility And Address-Space Preservation

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: extended prepared/BIR-to-target memory
operand conversion to pointer-value and string-constant bases where structured
facts permit it.

Concrete work completed:
- Pointer-value conversion now accepts structured BIR pointer addresses whose
  named `base_value` matches `PreparedAddress::pointer_value_name`.
- Pointer-value conversion joins the prepared pointer value name to
  `PreparedValueLocationFunction` and preserves the unique `PreparedValueId`;
  missing or ambiguous joins fail closed.
- String-constant conversion now accepts structured BIR string addresses whose
  identity matches the prepared string symbol, preserving `string_symbol_name`
  and available `string_name` / `TextId` without adding selection or emission.
- Frame-slot, global-symbol, pointer-value, and string-constant conversions
  share the existing fact validation for byte offset, available size,
  available alignment, volatility, address space, and access identity.
- Added focused `backend_aarch64_prepared_memory_operand_records` coverage for
  successful pointer-value store and string-constant load conversion plus
  guard cases for missing pointer home, ambiguous pointer home, pointer value
  mismatch, and string identity mismatch.

## Suggested Next

Execute Step 5 from `plan.md`: add memory instruction record wrappers around
the prepared memory operand records while preserving record-only ownership.

## Watchouts

- Keep this plan record-only; do not add load/store selection, assembly,
  encoding, object output, memory emission, calls, or returns.
- Preserve volatility and address-space facts from prepared input; do not
  invent target-local defaults.
- Pointer-value and string-constant support remains record-only; do not add
  load/store opcode selection, addressing-mode selection, assembly, encoding,
  object output, calls, or returns.
- Pointer conversion depends on a unique prepared value-location home for the
  base pointer. If later prep phases publish multiple homes for one value name,
  keep failing closed unless a stronger identity is added.
- String conversion currently validates BIR `StringConstant` address identity
  against prepared `LinkNameId` / string symbol spelling and preserves a
  `TextId` only when the prepared text table has one.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with
`backend_aarch64_prepared_memory_operand_records` pointer/string conversion
coverage included and green: 130 tests passed, 0 failed; 12 disabled MIR trace
tests were not run. Proof log path: `test_after.log`.
