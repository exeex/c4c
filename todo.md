Status: Active
Source Idea Path: ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Focused Local-Load Representative Evidence

# Current Packet

## Just Finished

Closed idea 422 as completed BIR semantic producer planning/selection work and
activated the selected follow-up idea 483, `BIR Semantic Local-Memory Scalar
Load Producer`.

Selection evidence:

- `build/agent_state/422_step2_bir_producer_buckets/bucket_table.tsv`
- `build/agent_state/422_step3_first_producer_packet/decision.md`
- `build/agent_state/422_step4_residual_disposition/disposition.md`

Selected bucket:

- local-memory load, 79 rows;
- first owner: BIR semantic local-memory load producer;
- representative rows: `src/20041124-1.c`, `src/20071219-1.c`,
  `src/991228-1.c`, `src/multi-ix.c`, `src/pr22098-1.c`.

## Suggested Next

Execute Step 1: audit focused local-load representative evidence. Identify the
narrowest ordinary scalar local-load shape that can support a producer
contract, and record adjacent shapes that must remain fail-closed.

## Watchouts

- Do not repair missing BIR semantic facts in RV64/MIR.
- Do not broaden this slice into GEP, store, call, memcpy/memset, alloca,
  bootstrap, aggregate/member, pointer/provenance, byval/va_arg, volatile,
  atomic, complex, vector, or F128 work.
- Do not infer producer facts from testcase names, value names, raw dump order,
  final homes, or target fallback behavior.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.

## Proof

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed.
