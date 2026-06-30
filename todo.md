Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-Probe Representatives And Decide Close Readiness

# Current Packet

## Just Finished

Step 4 re-probed representative rows and decided close readiness for the RV64
call-adjacent scalar inline-asm materialization plan.

Completed work:

- Probe artifacts are saved under
  `build/agent_state/428_step4_close_readiness/`; the concise classification is
  `build/agent_state/428_step4_close_readiness/probe_summary.tsv` and
  supporting snippets are in
  `build/agent_state/428_step4_close_readiness/README.md`.
- Probe command shape:
  `./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --dump-prepared-bir|--codegen obj <case>`.
- `pr38533`: prepared probe passes, `missing_operand0_home=no`,
  `tied_input_output_home_mismatch=no`, no unsupported memory/clobber marker,
  and RV64 `--codegen obj` now succeeds.
- `pr45695`: prepared probe passes, `missing_operand0_home=no`,
  `tied_input_output_home_mismatch=no`, no unsupported memory/clobber marker,
  and RV64 `--codegen obj` now succeeds.
- `pr49279`: prepared probe passes and the old tied-home producer facts are
  absent, but the row is a mixed scalar publication plus `~{memory}` clobber
  carrier. RV64 `--codegen obj` still rejects at
  `unsupported_instruction_fragment`; this is outside this plan by constraint.
- `pr40657`: sanity row remains the `=*m` memory-constraint class with
  `unsupported_constraint=yes`; RV64 `--codegen obj` still rejects at
  `unsupported_instruction_fragment`. This is outside this plan.
- `pr56982`: sanity row remains clobber-only `~{memory}` with no scalar
  publication; RV64 `--codegen obj` still rejects at
  `unsupported_instruction_fragment`. This is outside this plan.
- Acceptance coverage: coherent scalar GPR inline-asm carriers selected by this
  plan now have focused backend coverage and representative object-route
  evidence, including `.insn r`, `.insn d`, read/write `+r`, tied `.insn r`,
  substitution helpers, and empty-template tied `=r,0`.
- Close readiness: from this executor's evidence, the source idea appears ready
  for lifecycle close review. Remaining representative failures are explicitly
  out-of-scope memory/clobber classes and should not be folded into this plan.

## Suggested Next

Ask the plan owner to review lifecycle close readiness for
`ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`.

## Watchouts

- Do not key behavior to `pr38533`, `pr45695`, `pr49279`, or probe instruction
  indexes.
- Treat renewed `missing_operand0_home` or `tied_input_output_home_mismatch`
  evidence as a producer regression, not RV64 consumer work.
- Keep `=*m` memory constraints, mixed or clobber-only `~{memory}` carriers,
  frame-slot address arguments, symbol/address arguments, prior-preservation
  routing, aggregate ABI, vector, and F128 outside this plan unless a new
  source idea is created.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Proof passed and is saved in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
