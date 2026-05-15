Status: Active
Source Idea Path: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Select F32/F64 Arithmetic Nodes

# Current Packet

## Just Finished

Step 4 selected F32/F64 scalar arithmetic nodes from prepared FPR/SIMD
register authority without adding terminal printer output.

Completed work:

- `ScalarAluRecord` now distinguishes supported integer and supported floating
  operations, so F32/F64 selection does not masquerade as integer ALU support.
- Added explicit scalar FP/SIMD register-view handling for prepared F32/F64
  values beside `scalar_register_view`; F32 maps to S registers and F64 maps to
  D registers without treating floats as W/X.
- Prepared F32/F64 `Add`, `Sub`, `Mul`, and division-shaped BIR operations now
  select into structured scalar machine records with destination/source FPR
  register facts, operation kind, operand order, type width, and def/use facts.
- `Mul` and `Div` are now explicit scalar machine opcodes/operation kinds for
  selected records, but terminal printer mnemonics remain deferred.
- F128/binary128, missing storage, wrong-bank GPR-backed float facts, compare
  predicates, and unsupported non-arithmetic opcodes remain fail-closed.
- Focused tests cover raw scalar records, prepared F32/F64 conversion, dispatch
  selection from prepared FPR facts, and wrong-bank dispatch diagnostics.

## Suggested Next

Step 5 implementation packet: print the selected F32/F64 scalar arithmetic
subset from structured FP/SIMD destination/source register facts, without
adding F128/binary128 or FP cast output.

## Watchouts

- Terminal printer output for F32/F64 arithmetic is intentionally still absent;
  `Mul`/`Div` currently select as machine nodes but have no printable mnemonic
  mapping.
- Simple integer cast printing exists, but pointer, bitcast, float/int, and FP
  width conversion casts remain intentionally unsupported.
- Do not route F32/F64 through GPR raw-bit conventions or revive the archived
  accumulator/scratch approach.
- Keep the integer-only `scalar_register_view` behavior intact; use the
  adjacent F32/F64 view path for FP/SIMD register facts.
- F128/binary128 must remain rejected or delegated to the later soft-float
  route; do not lower F128 through scalar F64 paths.

## Proof

Fresh focused backend proof passed and wrote `test_after.log`:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_instruction_dispatch|backend_aarch64_scalar_record_contract)$') > test_after.log 2>&1`

Result: 4/4 focused backend MIR tests passed.
