Status: Active
Source Idea Path: ideas/open/421_rv64_instruction_fragment_bucket_classification.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Evidence Source And Bucket Inputs

# Current Packet

## Just Finished

Step 1 completed the evidence-source check for
`unsupported_instruction_fragment`. The handoff docs now record that no durable
row-level artifact existed under `docs/rv64_gcc_torture_post_contract/`, that
the mutable `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` inputs
are stale relative to the named scan logs, and that the exact row evidence
source should be a regenerated
`build/agent_state/unsupported_instruction_fragment_rows.tsv` created from the
command in `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`.

## Suggested Next

Execute Step 2 from `plan.md`: regenerate
`build/agent_state/unsupported_instruction_fragment_rows.tsv` with the recorded
command, then classify the rows in
`docs/rv64_gcc_torture_post_contract/failure_bucket_map.md` by BIR opcode,
operand/result type, prepared fact completeness, and likely owning layer.

## Watchouts

- Do not implement RV64 lowering in this classification runbook.
- Do not make F128 the primary route.
- Do not weaken expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Do not classify missing BIR/prepared producer facts as RV64 lowering work.
- Current local per-case logs contain 717
  `unsupported_instruction_fragment` hits, but that is only a stale-input
  sanity check until the row TSV is regenerated from a single matching scan.

## Proof

Passed: `git diff --check > test_after.log 2>&1`.

Proof log: `test_after.log`.
