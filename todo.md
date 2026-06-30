Status: Active
Source Idea Path: ideas/open/436_pr88904_sret_object_home_producer_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit PR88904 Prepared SRet Facts

# Current Packet

## Just Finished

Activated `ideas/open/436_pr88904_sret_object_home_producer_review.md` as the
next active plan. Selection rationale: it is the remaining concrete aggregate
producer review from the 431 series and must close before `pr88904` can be
used as coherent RV64 aggregate `sret` lowering evidence.

## Suggested Next

Execute Step 1: audit `pr88904` prepared `sret_param` object
size/alignment/object-home facts against call memory-return facts and classify
the first producer-contract packet.

## Watchouts

- Do not implement RV64 aggregate lowering for `pr88904` in this plan.
- Do not fold inline-asm, clobber, store-publication, select, or
  local-publication residuals into the `sret` producer review.
- Do not infer object layout, alignment, or home ownership in RV64 from source
  or testcase identity.
- Keep expectation, allowlist, unsupported-marker, runtime-comparison, and
  pass/fail accounting changes out of scope.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
