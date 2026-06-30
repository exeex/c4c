Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-Probe Representatives And Decide Close Readiness

# Current Packet

## Just Finished

Step 3 audited the remaining coherent scalar GPR inline-asm input/output
surface after Step 2 and found no additional in-scope implementation packet.

Completed work:

- Fresh local probes against the current working tree used
  `./build/c4cll -I tests/c/external/gcc_torture --target
  riscv64-linux-gnu --codegen obj` and `--dump-prepared-bir` for `pr38533`,
  `pr45695`, `pr49279`, `pr40657`, and `pr56982`.
- `pr38533`: prepared dump still shows complete empty-template `=r,0` scalar
  GPR carriers with no `missing_operand0_home` or
  `tied_input_output_home_mismatch`; RV64 object compilation now succeeds.
- `pr45695`: prepared dump still shows the complete empty-template `=r,0`
  scalar GPR carrier with `home=yes`; RV64 object compilation now succeeds.
- `pr49279`: prepared dump shows a coherent tied scalar GPR carrier, but it is
  mixed with `~{memory}` clobber metadata. RV64 object compilation still fails
  at `unsupported_instruction_fragment`; this remains outside the Step 3 scalar
  input/output packet by constraint.
- `pr40657`: prepared dump remains the `=*m` memory-constraint class with
  `unsupported_constraint0:=*m` and `unsupported_operand_constraint0`; this is
  out of scope.
- `pr56982`: prepared dump remains clobber-only `~{memory}` with no scalar
  publication; this is out of scope.
- Existing focused RV64 object-emission tests already cover the coherent
  scalar GPR inline-asm consumer paths selected by this plan: structured and
  text `.insn r`, `.insn d`, read/write `+r`, tied `.insn r`, substitution
  helpers, and the Step 2 empty-template tied `=r,0` route. No remaining
  coherent scalar GPR input/output carrier shape was found that fits this
  packet's constraints.

## Suggested Next

Execute Step 4: re-probe the representatives and decide close readiness for
the RV64 call-adjacent scalar inline-asm materialization plan.

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
