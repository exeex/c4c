Status: Active
Source Idea Path: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Typed Float/Integer Conversion Nodes

# Current Packet

## Just Finished

Step 5 selected typed float/integer and F32/F64 width conversion cast nodes
from prepared GPR/FPR register authority without adding terminal printer
output.

Completed work:

- `ScalarCastRecord` now has explicit conversion operation kinds for
  `FPExt`, `FPTrunc`, `SIToFP`, `UIToFP`, `FPToSI`, and `FPToUI`.
- Prepared conversion casts preserve source/result type widths, structured
  source/destination register operands, and explicit source/result prepared
  register-bank facts.
- GPR-to-FPR and FPR-to-GPR conversions record a bank transition through
  `crosses_register_bank`; F32/F64 width conversions remain within FPR/SIMD.
- F32/F64 conversion selection uses the existing prepared register conversion
  authority and the adjacent FP/SIMD view path, not W/X float shortcuts.
- `Bitcast`, pointer casts, F128/binary128, wrong-bank storage, missing
  register/storage facts, and unsupported conversion shapes remain fail-closed.
- Focused tests cover raw conversion vocabulary, prepared int/float and
  F32/F64 width conversion records, dispatch-selected bank transitions, and
  missing bank authority diagnostics.

## Suggested Next

Step 6 implementation packet: print selected F32/F64 arithmetic and conversion
nodes from structured register/type/bank-transition facts, without adding
F128/binary128 or unsupported pointer/bitcast output.

## Watchouts

- Terminal printer output for F32/F64 arithmetic and conversion nodes is
  intentionally still absent.
- Conversion machine records that have no dedicated terminal opcode currently
  carry selected scalar records with structured conversion payloads; printer
  work must use the `ScalarCastRecord` fields, not infer from rendered names.
- Simple integer cast printing exists, but pointer and bitcast casts remain
  intentionally unsupported.
- Do not route F32/F64 through GPR raw-bit conventions or revive the archived
  accumulator/scratch approach.
- Keep the integer-only `scalar_register_view` behavior intact; use the
  adjacent F32/F64 view path for FP/SIMD register facts.
- F128/binary128 must remain rejected or delegated to the later soft-float
  route; do not lower F128 through scalar F64 paths.

## Proof

Fresh focused backend proof passed and wrote `test_after.log`:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch|backend_aarch64_scalar_record_contract)$') > test_after.log 2>&1`

Result: 4/4 focused backend MIR tests passed.
