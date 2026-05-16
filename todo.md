# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Accepted Unary And Bit-Operation Routes

## Just Finished

Step 2 - Implement Accepted Unary And Bit-Operation Routes completed for the
first unary slice: integer negation, bitwise not, and count-leading-zero over
32-bit and 64-bit GPR views.

Added typed `ScalarUnaryRecord` support with `ScalarUnaryOperationKind`,
machine opcode identities for `neg`, `bit_not`, and `count_leading_zeros`,
selection through `ScalarInstructionRecord`, ALU-owned prepared-record helpers,
and printer spelling through allocation-backed result/source registers. The
slice does not add legacy `x0` accumulator authority and does not add a
named-case dispatch shortcut; current BIR has no unary instruction variant, so
the new prepared scalar-unary helper is the structured surface a later dispatch
route can call.

Focused tests now cover direct scalar unary records, prepared unary records
using storage-plan/value-home register facts, and printer spelling for 32-bit
and 64-bit `neg`, `mvn`, and `clz` forms.

## Suggested Next

Next implementation packet: continue Step 2 with CTZ and byte-swap as a second
unary/bit-operation slice. Extend the same `ScalarUnaryRecord` family rather
than adding a separate text-emitter path; model CTZ as structured `rbit` then
`clz`, and preserve the 16-bit byte-swap `rev` plus right-shift behavior
explicitly if 16-bit support is included. Suggested focused proof subset:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|machine_printer|instruction_dispatch|scalar_record_contract)$'
```

## Watchouts

- Do not reopen ALU markdown redistribution or recreate `alu.md`.
- Do not revive text-emitter accumulator conventions as semantic authority.
- Do not use named-case shortcuts, final assembly string matching, expectation
  rewrites, or unsupported downgrades to claim progress.
- Keep signed power-of-two division/remainder separate from unsigned reduction
  unless signed semantics are separately designed and proved.
- Preserve explicit 32-bit extension, scratch-conflict, and i128 high-half
  requirements when those routes are classified as accepted.
- Current integer scalar ALU support is narrower than the operation enum:
  `Add`, `Sub`, `And`, `Or`, and `Xor` are selected as integer operations, but
  the current printer path only accepts printable add/sub integer nodes.
- `Mul`, `SDiv`, `UDiv`, `SRem`, `URem`, and variable shifts currently map to
  operation names or BIR opcodes but are not accepted integer scalar ALU
  lowering/printer coverage.
- Do not treat old register names (`x0`, `x1`, `x2`, `x3`, `s0`, `d0`, `v0`)
  as allocation authority; they are only valid if selected by prepared
  allocation or explicit scratch facts.
- The current slice deliberately exposes an ALU-owned prepared unary helper
  without changing `dispatch.cpp`, because this packet did not own dispatch and
  current BIR has no unary instruction variant.
- Keep `emit_float_neg_impl` separate-idea material unless the source idea is
  changed to include scalar FP unary intrinsic/helper work.

## Proof

Proof passed and `test_after.log` was refreshed with the exact delegated proof
output.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|machine_printer|instruction_dispatch|scalar_record_contract)$'
```
