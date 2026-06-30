Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-Audit Remaining RV64 Object-Route Failures

# Current Packet

## Just Finished

Lifecycle close accepted for
`ideas/closed/432_prepared_inline_asm_operand_home_carriers.md` and this
runbook reactivated `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`.

The closed producer/carrier follow-up removed the old scalar GPR prepared facts
for `pr38533`, `pr45695`, and `pr49279`; remaining representative failures are
RV64 object-route `unsupported_instruction_fragment` failures owned by this
plan.

## Suggested Next

Execute Step 1: inspect `build/agent_state/432_step4_probes/classification.tsv`
and the representative object logs, classify the first coherent RV64
consumer-side scalar inline-asm materialization packet, and record the selected
target plus proof command here before code changes.

## Watchouts

- Do not key behavior to `pr38533`, `pr45695`, `pr49279`, or probe instruction
  indexes.
- Treat renewed `missing_operand0_home` or `tied_input_output_home_mismatch`
  evidence as a producer regression, not RV64 consumer work.
- Keep `=*m` memory constraints and clobber-only `~{memory}` carriers with no
  scalar publication outside this scalar GPR object-materialization plan unless
  the audit proves a shared RV64 consumer invariant requires them.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Lifecycle close and reactivation proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Run `git diff --check` before supervisor commit.
