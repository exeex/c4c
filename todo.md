Status: Active
Source Idea Path: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Simple Integer Cast Nodes

# Current Packet

## Just Finished

Step 2 selected prepared simple integer `bir::CastInst` nodes without adding
terminal printer output.

Completed work:

- `ScalarCastRecord` now carries structured result-register authority from
  prepared value-home/storage facts.
- `make_prepared_scalar_cast_record` resolves the cast destination through the
  same prepared storage-plan register conversion path used by scalar ALU
  records, preserving source/result type and source opcode facts.
- `make_scalar_cast_instruction_record` propagates the destination register to
  `ScalarInstructionRecord`, so selected cast machine nodes expose structured
  register defs and can feed later scalar/return consumers.
- `lower_scalar_instruction` now consumes `bir::CastInst` through
  `make_prepared_scalar_cast_instruction_record` and records emitted cast
  result registers in `BlockScalarLoweringState`.
- Unsupported FP casts, pointer casts, bitcasts, F128/i128 cases, and missing
  register facts remain fail-closed through existing prepared cast errors or
  scalar unsupported-instruction diagnostics.
- Focused tests cover selected `SExt`, `ZExt`, and `Trunc` records, destination
  register storage-placement preference, dispatch-selected cast nodes, and
  unsupported/missing-fact dispatch behavior.

## Suggested Next

Step 3 implementation packet: print selected simple integer cast nodes from
structured destination/source register and type facts, without adding FP cast
or FP arithmetic output.

## Watchouts

- Do not add pointer, bitcast, float/int, or FP width conversion semantics while
  printing the simple integer cast packet.
- Do not route F32/F64 through GPR raw-bit conventions or revive the archived
  accumulator/scratch approach.
- F32/F64 work needs explicit FP/SIMD record fields and S/D register views, not
  changes that make `scalar_register_view` treat floating types as W/X.
- F128/binary128 must remain rejected or delegated to the later soft-float
  route; do not lower F128 through scalar F64 paths.
- Step 2 intentionally left `MachineOpcode::SignExtend`, `ZeroExtend`, and
  `Truncate` unprintable; Step 3 owns those terminal forms.

## Proof

Fresh focused backend proof passed and wrote `test_after.log`:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch|backend_aarch64_scalar_record_contract)$') > test_after.log 2>&1`

Result: 3/3 focused backend MIR tests passed.
