# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Accepted Unsigned Power-Of-Two Reductions

## Just Finished

Step 3 - Implement Accepted Unsigned Power-Of-Two Reductions is complete for
the current structured scalar ALU surface.

Unsigned `UDiv` by a concrete immediate power-of-two greater than one now
prepares as a structured logical shift-right ALU record using allocation-backed
result/source registers and a rewritten shift-count immediate. Unsigned `URem`
by a concrete immediate power-of-two greater than one now prepares as a
structured mask through the existing `And` ALU record with a rewritten mask
immediate. The reduction path accepts direct BIR immediates and prepared
rematerialized immediates while preserving source value identity on the
rewritten immediate.

Focused tests cover direct record identity, prepared `UDiv` shift reduction,
prepared `URem` mask reduction, printer spelling for `lsr` and `and`, and
fail-closed behavior for signed `SDiv`/`SRem` and non-power unsigned
divisors. Narrow I8/I16 reduction and divisor-one behavior remain outside this
slice because they require explicit extension/zero-result semantics rather
than a plain I32/I64 ALU rewrite.

## Suggested Next

Next implementation packet: move to Step 4 scratch, fallback, and extension
routes. Start with one accepted subroute that has explicit current structured
facts, such as register-direct scratch handling or explicit post-operation
extension, and keep accumulator fallback division/remainder/variable-shift
behavior out unless current allocation and scratch authority are proven.
Suggested focused proof subset:

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
- Step 3 intentionally accepts only I32/I64 unsigned reductions with a concrete
  immediate divisor greater than one. I8/I16 and divisor-one cases still need
  explicit extension or zero-result semantics before being accepted.
- Preserve explicit 32-bit extension, scratch-conflict, and i128 high-half
  requirements when those routes are classified as accepted.
- Current integer scalar ALU support is narrower than the operation enum:
  `Add`, `Sub`, `And`, `Or`, and `Xor` are selected as integer operations, but
  the general bitwise printer path is still not accepted outside the Step 3
  unsigned-remainder mask route.
- `Mul`, signed `SDiv`/`SRem`, non-power unsigned `UDiv`/`URem`, divisor-one
  unsigned reductions, narrow I8/I16 reductions, and variable shifts currently
  map to operation names or BIR opcodes but are not accepted integer scalar
  ALU lowering/printer coverage.
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
