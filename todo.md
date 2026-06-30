Status: Active
Source Idea Path: ideas/open/464_select_carrier_alias_metadata.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 464. The prepared
carrier-alias metadata prerequisite is complete and close-ready as producer /
prepared authority work. No implementation files, tests, RV64 target lowering,
or logs were changed in this disposition packet.

Residual table:

| Residual question | Disposition |
| --- | --- |
| Durable carrier-alias surface | Complete: Step 3 added `PreparedSelectCarrierAliasAuthority*` records, planner, availability predicate, and collector in `publication_plans`. |
| Required keys | Covered: function, predecessor/successor edge, join/final result, selected source, binary source-producer site, carrier values, and optional carrier value ids. |
| Use closure | Covered: the planner proves the selected source-producer result has only authorized carrier alias uses. |
| Positive metadata coverage | Covered: duplicate carrier aliases are accepted directly and through `collect_prepared_select_carrier_alias_authorities` from a prepared module fixture. |
| Negative metadata coverage | Covered: missing source producer, raw-name-only/mismatched carrier result, and extra non-carrier source use stay fail-closed. |
| Exact remaining idea-464 metadata packet | None identified. |
| Later RV64 ULE rematerialization | May be reactivated only as a separate target-consumer packet that consumes `PreparedSelectCarrierAliasAuthorityRecords`; it must not infer aliases from raw `.phi.sel` names/shape. |

Artifact:
`build/agent_state/464_step4_residual_disposition/disposition.md`.

## Suggested Next

Plan-owner should close idea 464 as complete for prepared carrier-alias
metadata, or separately activate a later RV64 ULE rematerialization consumer
idea that explicitly consumes the new authority records.

## Watchouts

- Do not route RV64 ULE rematerialization back through idea 464; that is now a
  separate target-consumer concern.
- Do not infer duplicate-carrier authority from `%*.phi.sel*` spelling, raw
  select shape, value ids, block labels, function names, testcase names, or
  dump order.
- Preserve fail-closed behavior for extra non-carrier uses and mismatched
  source/destination/edge facts.
- Do not implement a plain `%t46 -> %t50` copy or same-register no-op.
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
