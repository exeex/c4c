Status: Active
Source Idea Path: ideas/open/value-home-storage-interpretation-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Prealloc Decoded Home/Storage API

# Current Packet

## Just Finished

Completed `Step 2: Add Prealloc Decoded Home/Storage API`.

Added a target-neutral decoded Prepared home/storage API in
`src/backend/prealloc/decoded_home_storage.hpp` and
`src/backend/prealloc/decoded_home_storage.cpp`.

The API now exposes:

- decoded source, kind, and status enums for Prepared home/storage facts;
- a decoded result payload for register placements, frame/stack slots,
  immediates, symbols, pointer-base-plus-offset facts, and missing-field
  states;
- individual helpers for regalloc assignments, storage-plan values, and
  value homes;
- a combined helper that preserves the current regalloc, storage-plan,
  value-home precedence without constructing MIR operands.

Repaired the combined helper so present-but-empty higher-precedence authority
records remain authority results and block lower-precedence fallback. True
no-record/no-authority lookups still fall through to the next authority source.

Added `backend_prealloc_decoded_home_storage` coverage for regalloc
register/stack assignments, storage-plan register/frame/immediate/symbol and
computed-address encodings, value-home stack/immediate/register and
pointer-base-plus-offset forms, missing-field statuses, no-authority results,
combined-helper precedence, present empty regalloc/storage-plan authority
records, and true no-record fallback.

## Suggested Next

Execute the next coherent packet by adapting AArch64 operand resolution to
consume the prealloc decoded home/storage API while keeping AArch64 register
conversion, MIR operand construction, immediate/addressing legality, and
diagnostics in AArch64 codegen.

## Watchouts

- The new prealloc API returns decoded facts only; it intentionally does not
  create target registers, MIR memory operands, immediates, or symbols.
- `PreparedValueHomeKind::Register` still decodes as a register-shaped fact
  with `MissingRegisterPlacement`; target code must not spell it as a physical
  register without typed placement authority.
- Storage-plan `ComputedAddress` and value-home `PointerBasePlusOffset` decode
  as unsupported forms with their facts preserved for target-specific handling.
- The combined helper treats missing fields as an authority result, so a
  higher-precedence malformed authority is not silently bypassed.

## Proof

Ran exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed out
of 151`; the new `backend_prealloc_decoded_home_storage` test passed.
