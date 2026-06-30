Status: Active
Source Idea Path: ideas/open/473_branch_site_stack_slot_materialization_no_clobber_source.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Source Fact Producer

# Current Packet

## Just Finished

Completed Step 3 as a routed/blocker packet. No prepared source-fact producer
was implemented because current producer/prepared data does not expose the
required explicit materialization/current-value and no-clobber facts without
raw-shape inference.

Checked surfaces:

| Surface | Current fact | Classification |
| --- | --- | --- |
| `PreparedBranchStackLoadAuthorityInputs` | Carries branch condition, terminator, value home, frame slot, stack object, policy, pointer status, freshness bool, and clobber-safety bool. | Downstream authority input only; not a source-fact carrier. |
| `collect_prepared_branch_stack_load_authorities` | Builds rows from prepared branch conditions, BIR terminators, value homes, frame slots, and stack objects. | Does not identify a materialization/write event or no-clobber proof. |
| Representative `%t23` row | Branch/load identity is visible for `f.logic.end.14`, `%t23`, slot `#21`, stack offset `156`, status `missing_policy`. | In scope, but missing the lower-level source facts needed by this plan. |
| Existing prepared evidence | Shows adjacent publications, move bundles, parallel copies, calls/effects, and boundary rows. | Useful evidence, but not a durable source-fact certificate. |

Exact blocker:

| Missing fact/carrier | Why Step 3 cannot implement safely |
| --- | --- |
| Prepared source-fact record/carrier | No existing record can publish materialization/current-value plus no-clobber source facts separately from downstream `PreparedBranchStackLoadAuthority`. |
| Exact materialization/write event | No producer fact names a concrete write of the current `%t23` value into frame slot `#21`. |
| Path/dominance validity | No fact proves the write reaches `logic.end.14` on all relevant paths. |
| Same-slot write exclusion | No summary proves no intervening write to slot `#21` between materialization and branch. |
| Call/helper/inline-asm safety | No effect summary proves intervening calls/helpers/inline asm cannot clobber slot `#21`. |
| Publication/move-bundle/parallel-copy non-clobber | Existing rows are visible but not classified as non-clobber proof for the slot. |

Artifacts:

- `build/agent_state/473_step3_materialization_no_clobber_source/blocker.md`
- `build/agent_state/473_step3_materialization_no_clobber_source/summary.md`

## Suggested Next

Execute Step 4 from `plan.md`: Residual Disposition And Close Readiness.
The next packet should decide whether idea 473 closes as blocked/routed to a
lower-level source-fact carrier/producer initiative, or remains active only if
one exact in-scope producer packet is identified.

## Watchouts

- Current Step 3 is routed/blocked; do not present it as implementation
  progress.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate available branch-stack-load authority records.
- Do not infer source facts from stack homes, offsets, object ids, raw BIR,
  value names, block labels, function names, testcase names, or dump order.
- Keep pointer-value/provenance, select-result stack-destination, and
  unsupported-terminator boundaries separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed out of 327`.
Log: `test_after.log`.
