Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize Remaining Coherent Scalar Inline-Asm Inputs/Outputs

# Current Packet

## Just Finished

Step 2 implemented RV64 object-route materialization for complete
empty-template tied scalar GPR inline-asm carriers.

Completed work:

- `src/backend/mir/riscv/codegen/object_emission.cpp` now recognizes only
  complete prepared inline-asm carriers with semantic empty templates,
  constraints `=r,0`, one GPR register output, one tied input, no named
  operands, no template modifiers, no clobbers, no missing facts, scalar GPR
  result type, and coherent shared prepared GPR homes.
- The consumer uses prepared carrier result homes, tied operand homes, and
  `tied_home_authority` as authority. It does not infer from representative
  filenames, probe instruction indexes, raw BIR, or non-empty asm text.
- Focused RV64 object-emission coverage proves the accepted generalized
  empty-template `=r,0` scalar GPR carrier materializes as a coherent object
  route. This gives the old `pr38533` / `pr45695` representative class a
  general consumer path.
- Focused fail-closed coverage preserves rejection for missing carriers,
  clobbered carriers such as the `pr49279` class, non-empty templates, named
  operands, template modifiers, missing result or tied homes, mismatched tied
  homes, vector homes, and F128 results.

## Suggested Next

Execute Step 3: audit and materialize any remaining coherent scalar GPR
inline-asm input/output fragments not covered by Step 2. Keep memory,
clobber-only, aggregate, vector, F128, pointer/address, and missing-prepared
fact shapes fail-closed.

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

Proof passed and is saved in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
