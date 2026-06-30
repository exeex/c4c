Status: Active
Source Idea Path: ideas/open/466_representative_select_carrier_alias_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Representative Authority Collection

# Current Packet

## Just Finished

Parked idea 465 after Step 4 found it is not representative-complete. Step 3
implemented a locally valid RV64 consumer for explicit
`PreparedSelectCarrierAliasAuthorityRecords`, but fresh `20010329-1` still
fails at the same coordinate because no matching carrier-alias authority record
is visible or matched for the real representative.

The new active idea 466 owns the producer/probe question:

| Field | Value |
| --- | --- |
| Representative | `20010329-1` |
| Object failure | Same `pre_terminator_copies` coordinate at `logic.rhs.end.40 -> logic.end.41` |
| Source producer | `%t46 = bir.ule ptr %t42, %t45` |
| Selected source / destination | `%t46 -> %t50` |
| Duplicate carriers | `%t50.phi.sel0` / `%t50.phi.sel1` |
| Missing evidence | No printed `carrier_alias` / `select_carrier` record in fresh prepared dump |
| First owner | Prove/publish/expose the matching carrier-alias authority record, or diagnose why collection/matching misses it |

## Suggested Next

Execute Step 1 from `plan.md`: audit the real `20010329-1` prepared module and
`collect_prepared_select_carrier_alias_authorities` path. Classify whether the
matching authority record is not produced, produced but not probe-visible, or
produced with fields the RV64 consumer does not match.

Suggested artifact directory:
`build/agent_state/466_step1_representative_authority_collection_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
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

Lifecycle repair proof:

```sh
git diff --check
python3 scripts/plan_review_state.py show
```

Result: passed.
