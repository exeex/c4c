Status: Active
Source Idea Path: ideas/open/99_bir_prealloc_control_publication_lookup_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Produce Follow-Up Ideas And Close Readiness Summary

# Current Packet

## Just Finished

Step 5 - Produce Follow-Up Ideas And Close Readiness Summary is complete.
I synthesized the Step 1-4 audit notes and created the only supported durable
follow-up idea:
`ideas/open/108_prepared_select_chain_dump_contract_coverage.md`.

### Final Classifications

| Item | Final classification | Step 5 disposition |
| --- | --- | --- |
| BIR CFG, terminators, phi/select/compare facts, instruction values | `bir-control-semantic-fact` | BIR owns these target-neutral semantics. No implementation follow-up from this audit. |
| Out-of-SSA join transfers, edge publications, current-block entry publications, store/source publication plans | `prealloc-transfer-plan-authority` | Retained as prepared target-facing transfer/publication authority. No BIR migration follow-up. |
| Select-chain and compare-join classification | retained prepared classification, not duplicated BIR semantic authority | No authority-overlap idea. The traced work prepares target-facing materialization/publication facts rather than replacing BIR CFG or expression semantics. |
| `find_select_materialization_join_transfer` | retained/no-overlap | No follow-up. The named lookup and prepared-printer output expose the select-materialization join-transfer carrier sufficiently for the current audit. |
| `find_prepared_compare_join_continuation_targets` | retained/no-overlap | No follow-up. Consumers use named lookup/helper paths, and prepared-printer output includes continuation targets. |
| `find_prepared_scalar_select_chain_materialization` | `prepared-printer-contract-gap`; lookup retained | Follow-up created in `ideas/open/108_prepared_select_chain_dump_contract_coverage.md` for dump and contract-test visibility of availability, root value, `root_is_select`, root instruction index, and related select-chain facts. |
| Direct-global select-chain dependency facts | `prepared-printer-contract-gap` | Follow-up created in `ideas/open/108_prepared_select_chain_dump_contract_coverage.md` for dump and contract-test visibility of `contains_direct_global_load`, root select state, root instruction index, and store/source publication dependency facts. |
| Source-producer kind/block/index facts | retained prepared provenance with supporting printer gap | Included only as supporting scope in `ideas/open/108_prepared_select_chain_dump_contract_coverage.md`, limited to source-producer provenance that explains scalar select-chain or direct-global dependency visibility. No separate idea. |
| Provisional `bir-missing-target-neutral-fact` leads from early steps | resolved to retained prepared publication facts or printer gaps | No remaining BIR-missing-fact follow-up. The surviving issue is contract visibility, not missing BIR semantics. |

### Generated Follow-Up Ideas

- `ideas/open/108_prepared_select_chain_dump_contract_coverage.md`: focused
  prepared-printer/contract-test coverage for scalar select-chain
  materialization, direct-global select-chain dependency, and directly
  supporting source-producer kind/block/index facts.

### No-Action Findings

- Do not create compare-join continuation work from this audit.
- Do not create select-materialization join-transfer lookup work from this
  audit.
- Do not create an arch-specific x86/RISC-V implementation idea from this
  audit; shared contract gaps have now been named first.
- Do not rewrite prepared lookup APIs based on this audit alone. The retained
  lookup issue is dump/test visibility for target-significant facts.

### Close Readiness

- All ordered plan steps have been completed.
- Every `needs-follow-up-idea` item has a corresponding path under
  `ideas/open/`; only `108_prepared_select_chain_dump_contract_coverage.md`
  was created.
- No implementation files were intentionally edited.
- The active source idea is ready for supervisor/plan-owner close evaluation as
  an analysis-only audit, with follow-up idea `108` remaining open.

## Suggested Next

Ask the plan owner to evaluate closure of
`ideas/open/99_bir_prealloc_control_publication_lookup_boundary_audit.md`.
The audit is complete; follow-up implementation work should proceed through
`ideas/open/108_prepared_select_chain_dump_contract_coverage.md` after the
current lifecycle state is closed or switched.

## Watchouts

- Close should preserve `108_prepared_select_chain_dump_contract_coverage.md`
  as an open follow-up idea.
- The close note should name that compare-join continuation and
  select-materialization join-transfer lookup coverage were no-action findings.
- This slice is analysis-only; any implementation diff under `src/backend/bir`
  or `src/backend/prealloc` should block acceptance.

## Proof

Delegated proof command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 5 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Result: passed. `test_after.log` contains the delegated Step 5 proof output:
`analysis-only Step 5 proof: no implementation changes under src/backend/bir or src/backend/prealloc`.
