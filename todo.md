Status: Active
Source Idea Path: ideas/open/432_prepared_inline_asm_operand_home_carriers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-Probe Representative Rows And Decide Close Readiness

# Current Packet

## Just Finished

Step 4 re-probed the representative rows and validated close readiness for the
prepared inline-asm operand home carrier plan.

Completed work:

- Fresh prepared probes are saved under
  `build/agent_state/432_step4_probes/`. The compact classification artifact is
  `build/agent_state/432_step4_probes/classification.tsv`.
- `pr38533`: prepared carrier probe succeeds with 303 coherent `=r,0` scalar
  GPR carriers. `missing_operand0_home=no` and
  `tied_input_output_home_mismatch=no`. The RV64 object-route probe still fails
  at `unsupported_instruction_fragment`, so remaining work belongs to the
  parked RV64 object materialization route, not this producer/carrier plan.
- `pr45695`: prepared carrier probe succeeds with one coherent `=r,0` scalar
  GPR carrier. `missing_operand0_home=no` and
  `tied_input_output_home_mismatch=no`. The RV64 object-route probe still fails
  at `unsupported_instruction_fragment`.
- `pr49279`: prepared carrier probe succeeds with one coherent tied scalar GPR
  carrier plus a `~{memory}` clobber operand. The old tied-home facts are gone;
  the remaining clobber shape is outside this scalar GPR plan and the RV64
  object-route probe still fails at `unsupported_instruction_fragment`.
- `pr40657`: prepared probe keeps the existing out-of-scope `=*m` memory
  constraint classification as `unsupported_constraint0:=*m` and
  `unsupported_operand_constraint0`; the RV64 object-route probe still fails at
  `unsupported_instruction_fragment`.
- `pr56982`: prepared probe records a clobber-only `~{memory}` carrier with no
  scalar GPR publication. This remains outside this scalar GPR plan, and the
  RV64 object-route probe still fails at `unsupported_instruction_fragment`.
- Close readiness: this idea is ready for lifecycle close from the executor
  perspective. The producer/carrier blocker facts targeted by this idea are
  gone for the coherent tied scalar GPR representatives. The parked RV64
  call-adjacent idea should be reconsidered as the owner of remaining RV64
  object-route materialization failures; it should not be closed by this
  producer/carrier validation alone.

## Suggested Next

Ask the plan owner to close or retire this completed prepared inline-asm
operand home carrier plan, then decide whether to reactivate the parked RV64
call-adjacent scalar inline-asm materialization idea for the remaining
`unsupported_instruction_fragment` object-route failures.

## Watchouts

- Step 4 did not change implementation, expectations, allowlists, unsupported
  markers, runtime comparison, or pass/fail accounting.
- The remaining representative RV64 object-route failures are not evidence of
  missing prepared operand homes; they are still parked object-materialization
  work.
- Do not key behavior to `pr38533`, `pr45695`, `pr49279`, or probe instruction
  indexes.
- Keep `=*m` memory constraints and clobber-only `~{memory}` carriers outside
  this scalar GPR producer/carrier plan unless the audit proves a shared
  invariant requires them.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Proof passed and is saved in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
