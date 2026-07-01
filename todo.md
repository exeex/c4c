Status: Active
Source Idea Path: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Local-Address Array-Element Authority Evidence

# Current Packet

## Just Finished

Closed idea 483 as a routed blocker and activated prerequisite idea 484,
`BIR Semantic Local-Address Provenance And Array-Element Access Authority`.

Reason:

- corrected Step 1 for idea 483 searched the full 79-row local-memory load
  bucket;
- no non-pointer/provenance ordinary scalar local-object load representative
  was found;
- representative rows require local object, array decay, index/offset, element
  layout, and pointer-to-local-element provenance facts before scalar load
  production can safely proceed.

Key evidence:

- `build/agent_state/483_step1_corrected_local_load_search/audit.md`
- `build/agent_state/483_step1_corrected_local_load_search/local_load_rows.tsv`
- `review/483_step1_local_load_route_review.md`

## Suggested Next

Execute Step 1: audit local-address array-element authority evidence. Decide
which representative shape is bounded enough for a producer contract, starting
with `pr22098-1.c`, `pr38048-1.c`, and `multi-ix.c` only as evidence sources,
not testcase-specific implementation targets.

## Watchouts

- Do not implement scalar local-load consumption in this idea.
- Do not accept integer-pointer round-trip provenance.
- Do not infer local-address authority from testcase names, value names, raw
  dump order, final homes, or RV64 target fallback behavior.
- Keep global, aggregate/member, variadic/va_arg, runtime/call, F128,
  bootstrap, and unsupported provenance shapes fail-closed.
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
