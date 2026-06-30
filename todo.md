Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize Coherent Tied Scalar GPR Inline-Asm Rows

# Current Packet

## Just Finished

Step 1 re-audited the remaining RV64 object-route failures after the prepared
inline-asm operand-home carrier plan closed.

Evidence used:

- `build/agent_state/432_step4_probes/classification.tsv`
- `build/agent_state/432_step4_probes/pr38533.object.log`
- `build/agent_state/432_step4_probes/pr45695.object.log`
- `build/agent_state/432_step4_probes/pr49279.object.log`
- representative prepared dumps under
  `build/agent_state/432_step4_probes/*.prepared.out`

Classification:

- `pr38533`: prepared probe succeeds with 303 complete scalar GPR `=r,0`
  inline-asm carriers; `missing_operand0_home=no` and
  `tied_input_output_home_mismatch=no`. The RV64 object route still rejects at
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`. This is in scope for the consumer-side empty-template tied
  scalar GPR publication packet.
- `pr45695`: prepared probe succeeds with one complete scalar GPR `=r,0`
  carrier with `value=%p.x` and `home=yes`; both old producer facts are absent.
  The object route still rejects at the same `unsupported_instruction_fragment`
  boundary. This is in scope for the same empty-template tied scalar GPR
  publication packet.
- `pr49279`: prepared probe succeeds with one complete tied scalar GPR carrier,
  but the carrier also has `~{memory}` clobber metadata. The old tied-home facts
  are absent, and the object route still rejects at
  `unsupported_instruction_fragment`. Keep this out of the first packet because
  the existing RV64 inline-asm object route rejects clobbers and this plan
  should not silently broaden into memory-clobber semantics.
- `pr40657`: remains out of scope as the `=*m` memory constraint class
  (`unsupported_constraint0:=*m`, `unsupported_operand_constraint0`).
- `pr56982`: remains out of scope as clobber-only `~{memory}` with no scalar
  publication.

First implementation target:

- Implement RV64 object-route consumer materialization for complete
  empty-template scalar GPR tied inline-asm carriers: a coherent `=r,0`
  register output plus tied input, no named operands, no template modifiers, no
  clobbers, scalar GPR result home, and materializable scalar source value.
- Primary owned files for the implementation packet:
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`, `todo.md`, and
  `test_after.log`.
- Representative accepted rows/classes: `pr38533`, `pr45695`, and generalized
  complete empty-template `=r,0` scalar GPR prepared carriers.
- Representative rejected/out-of-scope rows/classes: `pr49279` mixed
  `~{memory}` clobber, `pr40657` `=*m` memory constraint, `pr56982`
  clobber-only no-publication carrier, named/template-modifier inline asm,
  aggregate, vector, F128, pointer/address publication, expectations,
  allowlists, unsupported markers, runtime comparison, and pass/fail
  accounting changes.

## Suggested Next

Execute Step 2: lower complete empty-template tied scalar GPR inline-asm
publication in the RV64 object route. Use prepared inline-asm carrier facts and
prepared value homes as authority; do not infer from representative filenames
or instruction indexes.

## Watchouts

- Do not key behavior to `pr38533`, `pr45695`, `pr49279`, or probe instruction
  indexes.
- Treat renewed `missing_operand0_home` or `tied_input_output_home_mismatch`
  evidence as a producer regression, not RV64 consumer work.
- Keep `=*m` memory constraints, mixed or clobber-only `~{memory}` carriers,
  frame-slot address arguments, symbol/address arguments, prior-preservation
  routing, aggregate ABI, vector, and F128 outside the first consumer packet.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 1 audit proof:

```sh
git diff --check
```

Later implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```
