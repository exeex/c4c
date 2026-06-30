# RV64 gcc_torture Failure Bucket Map

Status: Step 1 intake complete; Step 2 should populate concrete sub-buckets.

## Evidence Source For `unsupported_instruction_fragment`

Use `docs/rv64_gcc_torture_post_contract/current_scan_summary.md` as the
evidence-source decision for this bucket. Step 2 should classify rows from:

- `build/agent_state/unsupported_instruction_fragment_rows.tsv`

That file must be regenerated from the exact command in
`current_scan_summary.md` before counts are treated as current.

## Current Bucket State

`unsupported_instruction_fragment` is not yet decomposed. The next packet should
group regenerated rows by:

- BIR opcode.
- Operand and result type.
- Prepared fact completeness.
- Likely owner: ordinary RV64 lowering, scalar F32/F64 lowering, F128
  quarantine, or BIR/prepared producer gap.

## Step 1 Input Notes

- The existing docs directory previously contained only the artifact README, so
  no durable row-level artifact was available under the handoff path.
- Local case logs currently show `unsupported_instruction_fragment` occurrences,
  but the mutable summary files are stale relative to the named full-scan logs.
- Classification must cite one timestamped scan log and its matching row TSV.
