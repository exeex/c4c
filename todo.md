# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Accepted i128 Copy Behavior

## Just Finished

Step 4 route review completed. The behavior-consuming zero-extension and
sign-extension subroutes remain valid Step 4 progress: their structured facts
are consumed by printer behavior and have fail-closed coverage for unsupported
extension facts.

The remaining scratch/overlap/fallback scope is parked before more execution.
Register-direct scratch handling, right-side result-register overlap, and
accumulator fallback division/remainder/variable-shift behavior must not
continue as Step 4 metadata-only classification work. Any future packet in that
area needs a lifecycle-reviewed split that first defines prepared scratch and
allocation authority, then proves a concrete backend behavior consumes that
authority.

## Suggested Next

Next implementation packet: proceed to Step 5, `Implement Accepted i128 Copy
Behavior`, unless the supervisor chooses to activate a separate
scratch/allocation authority initiative first.

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
- Do not resume Step 4 scratch/overlap/fallback work without a
  lifecycle-reviewed split that grants or designs prepared scratch/allocation
  authority first.
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

Lifecycle-only rewrite. No implementation files changed and no new build was
run by the plan owner. The supervisor refreshed `test_after.log` before this
rewrite after the route review noted it missing.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|machine_printer|instruction_dispatch|scalar_record_contract)$'
```
