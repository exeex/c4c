# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Accepted Unary And Bit-Operation Routes

## Just Finished

Step 2 - Implement Accepted Unary And Bit-Operation Routes continued with the
second unary slice: count-trailing-zero and byte-swap through the existing
structured scalar unary surface.

Extended `ScalarUnaryOperationKind` and machine opcode identity with
`count_trailing_zeros` and `byte_swap`, using the existing
`ScalarUnaryRecord`, `ScalarInstructionRecord`, ALU-owned prepared-record
helpers, and allocation-backed result/source registers. CTZ now prints as the
structured AArch64 `rbit` then `clz` sequence. Byte-swap prints width-specific
`rev` forms, including explicit 16-bit `rev w*, w*` followed by
`lsr w*, w*, #16`.

Focused tests now cover direct scalar unary records, prepared unary records
using storage-plan/value-home register facts, CTZ printer spelling for 32-bit
and 64-bit forms, and byte-swap printer spelling for 16-bit, 32-bit, and
64-bit forms. This remains structured record/prepared/printer progress, not an
end-to-end dispatch route.

## Suggested Next

Next implementation packet: either finish Step 2 with popcount only if a
prepared SIMD/FPR temporary policy is available in the current surfaces, or
move to Step 3 for unsigned power-of-two `UDiv`/`URem` reductions. Do not
implement popcount by hard-coding legacy `v0`/`s0` scratch authority. Suggested
focused proof subset for either route:

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
- Popcount still needs explicit scratch/temporary authority before
  implementation; do not revive the legacy fixed `v0`/`s0` sequence as
  allocation authority.

## Proof

Proof passed and `test_after.log` was refreshed with the exact delegated proof
output.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|machine_printer|instruction_dispatch|scalar_record_contract)$'
```
