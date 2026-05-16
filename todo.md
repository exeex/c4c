# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Accepted Scratch, Fallback, And Extension Routes

## Just Finished

Step 3 - Implement Accepted Unsigned Power-Of-Two Reductions is complete for
the current structured scalar ALU surface.

The attempted Step 4 overlap slice was rejected by
`review/aarch64_alu_step4_overlap_route_review.md` and restored before this
repair. The rejected route computed RHS/result overlap metadata but no
selection, emission, def/use, or unsupported-path behavior consumed that fact,
so it was classification-only progress against a semantic-lowering step.

Step 4 remains active, but its next packet must not reintroduce a standalone
overlap flag as the accepted result. The next slice must either make a
structured conflict or extension fact drive real behavior with fail-closed
proof, or document that current structured facts are insufficient and return
for a split/blocker decision.

## Suggested Next

Next implementation packet: Step 4 scratch, fallback, and extension routes.
Start with one accepted subroute that has explicit current structured facts and
whose fact is consumed by backend behavior.

Acceptable Step 4 packet shapes:
- Conflict-consuming route: for RHS/result overlap, make selection, emission,
  def/use modeling, or unsupported-path handling explicitly consume the
  conflict/safety fact. Prove the positive accepted case and at least one
  unsafe or unsupported overlap case that fails closed instead of becoming
  implicit scratch authority.
- Extension-consuming route: model signed 32-bit extension or unsigned
  zero-extension as explicit post-operation behavior for a route that requires
  it. Prove the extension instruction/record behavior and a negative case that
  does not silently reuse plain I32/I64 ALU lowering.
- Blocker/split route: if current allocation, scratch, or typed record facts
  cannot express a safe conflict/extension decision, stop and record the exact
  missing fact or abstraction needed so the supervisor can split or reroute the
  source idea.

Do not claim Step 4 progress for a helper, record field, or classification bit
unless a backend decision consumes it. Keep accumulator fallback
division/remainder/variable-shift behavior out unless current allocation and
scratch authority are proven.
Suggested focused proof subset:

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

No code validation was run for this lifecycle-only route repair. The next
executor packet should refresh `test_after.log` with the exact delegated proof
command after implementing behavior-consuming Step 4 work.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|machine_printer|instruction_dispatch|scalar_record_contract)$'
```
