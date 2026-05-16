# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Accepted Scratch, Fallback, And Extension Routes

## Just Finished

Step 4 - Implement Accepted Scratch, Fallback, And Extension Routes completed
one additional extension-consuming subroute: prepared I32 Add/Sub records with
I64 results now carry an explicit post-sign-extension width, and the scalar ALU
printer consumes that fact by emitting the arithmetic operation in W view
followed by `sxtw` into the structured X-result register. Invalid or
non-32-bit sign-extension facts fail closed in the printer, and bitwise
I32-to-I64 ALU records do not silently claim signed-extension semantics.

## Suggested Next

Next implementation packet: continue Step 4 only if the supervisor wants to
attempt the remaining conflict-consuming scratch/overlap route, or send the
plan to review if two extension-consuming subroutes are enough for this step.
Keep accumulator fallback division/remainder/variable-shift behavior out unless
current allocation and scratch authority are proven.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|machine_printer|instruction_dispatch|scalar_record_contract)$'
```

## Watchouts

- Do not reopen ALU markdown redistribution or recreate `alu.md`.
- Do not revive text-emitter accumulator conventions as semantic authority.
- Do not use named-case shortcuts, final assembly string matching, expectation
  rewrites, or unsupported downgrades to claim progress.
- Do not accept metadata-only overlap classification as Step 4 progress. A
  conflict or extension fact must gate, reject, repair, select, emit, or model
  a concrete backend behavior.
- Add fail-closed proof for unsafe or unsupported overlap/extension cases; a
  happy-path printable register shape alone is insufficient.
- Keep signed power-of-two division/remainder separate from unsigned reduction
  unless signed semantics are separately designed and proved.
- Step 4 now accepts I8/I16 unsigned power-of-two reductions only through the
  explicit post-zero-extension route. Divisor-one cases still need zero-result
  or identity semantics before being accepted.
- Preserve explicit 32-bit extension, scratch-conflict, and i128 high-half
  requirements when those routes are classified as accepted.
- Current integer scalar ALU support is narrower than the operation enum:
  `Add`, `Sub`, `And`, `Or`, and `Xor` are selected as integer operations, but
  the general bitwise printer path is still not accepted outside the Step 3
  unsigned-remainder mask route.
- `Mul`, signed `SDiv`/`SRem`, non-power unsigned `UDiv`/`URem`, divisor-one
  unsigned reductions, and variable shifts currently map to operation names or
  BIR opcodes but are not accepted integer scalar ALU lowering/printer
  coverage.
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

Passed. `test_after.log` contains the delegated proof output for the signed
I32 post-operation extension subroute.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|machine_printer|instruction_dispatch|scalar_record_contract)$'
```
