Status: Active
Source Idea Path: ideas/open/431_prepared_aggregate_abi_contract_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Aggregate ABI Evidence And Choose First Review Packet

# Current Packet

## Just Finished

Activated `ideas/open/431_prepared_aggregate_abi_contract_review.md` as the next
active RV64 post-contract plan. Selection rationale: after closing 430, the
original follow-up order names prepared aggregate ABI contract review as the
remaining high-impact post-contract candidate, and it must run before any RV64
aggregate lowering.

## Suggested Next

Execute Step 1: audit aggregate `sret`/`byval` representative evidence, classify
prepared ABI fact coherence, and choose the first bounded review packet.

## Watchouts

- Do not implement RV64 aggregate lowering in this review plan.
- Do not treat aggregate rows as scalar call publication.
- Do not infer missing aggregate size/alignment/storage/return slot/byval copy
  facts in RV64.
- Keep F128, long-double, runtime-helper policy, expectation changes, and
  pass/fail accounting out of this plan.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
