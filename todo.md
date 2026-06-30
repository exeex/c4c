Status: Active
Source Idea Path: ideas/open/466_representative_select_carrier_alias_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Representative Authority Packet

# Current Packet

## Just Finished

Completed Step 3 implementation for idea 466. Prepared evidence now exposes
select carrier-alias authority rows without changing RV64 lowering: available
records remain the only authority consumed by RV64, while the new evidence
collector/printer also reports unavailable per-publication rejection statuses.

Implementation table:

| Surface | Result | Boundary |
| --- | --- | --- |
| Evidence collector | Added `collect_prepared_select_carrier_alias_authority_evidence` and `PreparedSelectCarrierAliasAuthorityEvidence`. | Existing `collect_prepared_select_carrier_alias_authorities` still filters to available records only. |
| Prepared printer | Added row-gated `prepared-select-carrier-alias-authorities` dump output. | Empty modules and modules without evidence rows keep existing dump shape. |
| Focused tests | Covered available duplicate-carrier authority printing and rejected `non_carrier_source_use` evidence that stays non-authority. | No RV64 lowering/test expectation changes. |
| Representative probe | Fresh `20010329-1` dump now prints the real `%t46 -> %t50` row as `status=unsupported_carrier_alias` with `carrier_alias_candidates=2`. | The record is rejected before becoming available authority; it is not hidden and not RV64-mismatched. |

Artifact:
`build/agent_state/466_step3_representative_authority_evidence/summary.md`.

## Suggested Next

Execute Step 4 residual disposition. The representative is now classified as
rejected by the producer/planner with `unsupported_carrier_alias`, so Step 4
should decide whether idea 466 is close-ready as evidence/probe work or should
remain active/split a precise follow-up to diagnose or repair that rejection.

## Watchouts

- Do not classify the representative as solved from raw duplicate-carrier
  select shape.
- Unavailable status rows are diagnostics only; they are not RV64 authority.
- Do not route back to RV64 consumer work yet; the representative record is
  rejected as `unsupported_carrier_alias`, not available with mismatched RV64
  fields.
- Any follow-up must explain why two candidate aliases for `%t46 -> %t50` are
  rejected before authority publication.
- Do not make RV64 ULE rematerialization changes until representative
  authority is proven present and matchable.
- Do not infer aliases from `%*.phi.sel*` spelling, raw select shape, value
  ids, block labels, function names, testcase names, or dump order.
- Do not implement plain `%t46 -> %t50` copies or same-register no-ops.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
