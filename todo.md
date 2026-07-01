Status: Active
Source Idea Path: ideas/open/485_bir_local_array_address_derivation_index_range_authority_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Local Array Carrier Inputs

# Current Packet

## Just Finished

Closed idea 484 as a negative/blocked producer result and activated prerequisite
idea 485, `BIR Local Array Address Derivation And Index-Range Authority
Carrier`.

Reason:

- idea 484 defined the higher-level local-address provenance contract;
- Step 3 proved current BIR/HIR route-local lowering data cannot implement it
  without raw-shape inference;
- the lower-level missing owner is durable local array source-object,
  derivation, element-path, index-range, layout/range, and provenance carrier
  data.

Key evidence:

- `build/agent_state/484_step3_local_address_authority_producer/blocker.md`
- `build/agent_state/484_step4_residual_disposition/disposition.md`

## Suggested Next

Execute Step 1: audit local array carrier inputs. Start with `pr38048-1.c` as
the first accepted blocked shape, then classify which existing local-memory
lowering facts are durable producer facts and which are only route-local aids.

## Watchouts

- Do not implement scalar local-load consumption or idea 484 packaging in this
  idea.
- Do not accept integer-pointer round-trip provenance.
- Do not infer authority from route-local `element_slots`, `base_index`,
  lowered index values, type text, testcase names, value names, dump order,
  final homes, or target fallback behavior.
- Keep global, aggregate/member, variadic/runtime, F128, complex/vector,
  volatile/atomic, bootstrap, raw-shape-only, and target-only cases
  fail-closed.
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
