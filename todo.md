Status: Active
Source Idea Path: ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Generic Immediate Phi-Join Rows

# Current Packet

## Just Finished

Completed plan Step 1 classification for generic immediate phi-join rows from
the 504 handoff. The only matching row is `src/pr37924.c`, sourced from
`build/agent_state/504_step3_select_publication_consumer_classification/cases/src_pr37924.c.log`,
with `event_kind=pre_terminator_copies`, `phase=block_entry`,
`authority=out_of_ssa_parallel_copy`, `move[0].reason=phi_join_immediate_materialization`,
`move[0].destination_storage=register`, explicit edge coordinates
(`parallel_copy_predecessor=tern.then.end.24`,
`parallel_copy_successor=tern.end.27`,
`parallel_copy_execution_site=predecessor_terminator`), and explicit immediate
fact `move[0].source_imm_i32=0`.

This is a coherent generic immediate-to-register out-of-SSA phi-join row. The
classification is based only on prepared diagnostic facts, not testcase names,
source snippets, raw BIR order, object output, or target register behavior.
Focused evidence is recorded in
`build/agent_state/506_step1_generic_immediate_phi_join_classification/summary.md`
and `build/agent_state/506_step1_generic_immediate_phi_join_classification/rows.tsv`.

## Suggested Next

Execute plan Step 2 as a narrow RV64 implementation packet for generic
prepared move-bundle materialization of explicit
`phi_join_immediate_materialization` moves to register destinations, gated on
prepared `out_of_ssa_parallel_copy` authority, explicit edge coordinates, and
explicit `source_imm_i32` facts.

## Watchouts

- Do not use select-publication evidence to justify generic immediate
  materialization.
- Do not infer constants from testcase names, source snippets, raw BIR shape,
  object output, or target register behavior.
- Keep missing immediate facts, missing or ambiguous edge coordinates,
  unsupported destination storage, and any non-`source_imm_i32` immediate shape
  fail-closed unless a later packet proves a prepared semantic fact for it.
- The 504 handoff evidence contains no additional generic immediate phi-join
  rows beyond `src/pr37924.c`; the implementation must still be semantic and
  must not special-case that source file, function, block label, or immediate
  value.

## Proof

Delegated proof command:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
