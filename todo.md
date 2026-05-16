# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Accepted i128 Copy Behavior

## Just Finished

Step 5 implemented accepted i128 copy behavior as structured AArch64 MIR
transport. `bitcast i128 -> i128` now lowers through a
`CopyRegisterPair` i128 transport record carrying separate prepared destination
and source low/high lane facts. The printer emits low-lane and high-lane moves
from those record fields, and the selection/effect records expose two lane
defs and two lane uses without memory side effects.

## Suggested Next

Next packet: start Step 6 acceptance review and broader proof for the accepted
ALU semantic routes, unless the supervisor wants an independent route review
of the Step 5 i128 copy transport first.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch|target_record_core_contract)$'
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
- Step 5 treats i128 copy as same-width `Bitcast` only. Other i128-producing
  casts remain outside this packet and should stay fail-closed unless a later
  plan step accepts them.
- The i128 copy printer deliberately emits independent low/high register moves.
  If overlap-sensitive parallel-copy behavior is needed later, route it through
  the parked scratch/allocation authority work instead of weakening this record.

## Proof

The supervisor-selected Step 5 proof passed and was written to `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch|target_record_core_contract)$'
```
