Status: Active
Source Idea Path: ideas/open/466_representative_select_carrier_alias_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 466. The idea is close-ready as
representative authority evidence/probe work: the real `20010329-1`
`%t46 -> %t50` row is no longer hidden and is not RV64-mismatched; it is
explicitly rejected before becoming authority with `status=unsupported_carrier_alias`.

Residual table:

| Item | Evidence | Disposition |
| --- | --- | --- |
| Prepared evidence | `20010329-1.prepared.status=0`; dump prints `select_carrier_alias_authority function=main status=unsupported_carrier_alias predecessor=logic.rhs.end.40 successor=logic.end.41 destination=%t50 destination_value_id=21 source=%t46 source_value_id=20 source_producer=binary source_producer_block=logic.end.41 source_producer_inst=1 carrier_alias_candidates=2 carrier_aliases=0 source_use_closure=no`. | Representative record is visible as rejected evidence. |
| Hidden/missing/mismatched question | Evidence row exists but is unavailable. | Not hidden; not produced as authority; no RV64 field-mismatch route yet. |
| Object route | `20010329-1.object.status=2`, still `unsupported_move_bundle_target_shape ... from_value_id=20 to_value_id=21`. | Expected because unavailable evidence rows are diagnostics only. |
| Adjacent rows | Immediate edge reports `unsupported_publication`; earlier select edges report `missing_carrier_aliases`. | Out of this exact representative owner. |
| First owner | `unsupported_carrier_alias` despite `carrier_alias_candidates=2`. | Follow-up belongs to prepared carrier-alias planner/producer rejection analysis. |

Artifact:
`build/agent_state/466_step4_residual_disposition/disposition.md`.

## Suggested Next

Plan-owner should close idea 466 as complete for evidence/probe classification
after this slice is accepted. The next implementation route, if selected,
should be a separate precise follow-up for the prepared
`unsupported_carrier_alias` planner rejection on `%t46 -> %t50`; it should
diagnose why two candidates are collected but rejected before authority
publication.

## Watchouts

- Do not classify the representative as solved from raw duplicate-carrier
  select shape.
- Unavailable status rows are diagnostics only; they are not RV64 authority.
- Do not route back to idea 465/RV64 consumer work until an available
  carrier-alias authority record exists or a follow-up proves an RV64 matcher
  mismatch against an available record.
- The exact follow-up owner is prepared carrier-alias planner/producer
  rejection analysis for `unsupported_carrier_alias`.
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

Step 4 proof:

```sh
git diff --check
```

Result: passed.
